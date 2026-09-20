#define _POSIX_C_SOURCE 200809L
#include "asr.h"
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static volatile sig_atomic_t stopped;
static void stop(int sig) { (void)sig; stopped=1; }
static double now(void) {
    struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t);
    return t.tv_sec+t.tv_nsec/1e9;
}
static int capture(const char *program,const char *device,const char *output) {
    pid_t child=fork();
    if(child<0) return -1;
    if(child==0) {
        execl(program,program,"--interactive","--device",device,"--output",output,(char *)NULL);
        _exit(127);
    }
    int status;
    for(;;) {
        if(stopped) kill(child,SIGTERM);
        if(waitpid(child,&status,0)>=0) break;
        if(errno!=EINTR) return -1;
    }
    return WIFEXITED(status)?WEXITSTATUS(status):130;
}
int main(int argc,char **argv) {
    if(argc!=5) {
        fprintf(stderr,"Usage: asr_session MODEL_DIR RECORDER DEVICE OUTPUT_ROOT\n"); return 2;
    }
    if(!isatty(STDIN_FILENO)) {fprintf(stderr,"A terminal is required.\n");return 2;}
    struct sigaction sa={0}; sa.sa_handler=stop; sigemptyset(&sa.sa_mask);
    sigaction(SIGINT,&sa,NULL); sigaction(SIGTERM,&sa,NULL);
    char model[4096],tokens[4096],session[4000],wavpath[4096],txtpath[4096],logpath[4096],csvpath[4096],error[256];
    if(snprintf(model,sizeof(model),"%s/model.int8.onnx",argv[1])>=(int)sizeof(model) ||
       snprintf(tokens,sizeof(tokens),"%s/tokens.txt",argv[1])>=(int)sizeof(tokens) ||
       snprintf(session,sizeof(session),"%s/session_XXXXXX",argv[4])>=(int)sizeof(session)-64) return 2;
    if(access(model,R_OK) || access(tokens,R_OK)) {perror("Model files");return 3;}
    if(!mkdtemp(session)) {perror("Create session directory");return 4;}
    snprintf(logpath,sizeof(logpath),"%s/metrics.tsv",session);
    snprintf(csvpath,sizeof(csvpath),"%s/summary.csv",session);
    FILE *log=fopen(logpath,"wx");
    if(!log) {perror("Metrics");return 4;}
    FILE *csv=fopen(csvpath,"wx");
    if(!csv) {perror("Summary");fclose(log);return 4;}
    double load;
    fprintf(stderr,"LOADING MODEL...\n");
    AsrEngine *engine=asr_engine_create(model,tokens,2,&load,error,sizeof(error));
    if(!engine) {fprintf(stderr,"%s\n",error);fclose(csv);fclose(log);return 5;}
    fprintf(stderr,"model_load_seconds=%.6f (once)\nSession: %s\n",load,session);
    fprintf(log,"# model_load_seconds=%.6f\nround\tstatus\taudio_seconds\tdecode_seconds\tpost_capture_seconds\tround_seconds\n",load);
    fprintf(csv,"round,status,audio_seconds,decode_seconds,post_capture_seconds,round_seconds,wav_file,text_file\n");
    fflush(log);
    fflush(csv);
    for(unsigned round=1;!stopped;++round) {
        snprintf(wavpath,sizeof(wavpath),"%s/round_%06u.wav",session,round);
        snprintf(txtpath,sizeof(txtpath),"%s/round_%06u.txt",session,round);
        fprintf(stderr,"\nROUND %u\n",round);
        double begin=now(),post=0,duration=0,decode=0,post_end=0,round_end=0;
        int rc=capture(argv[2],argv[3],wavpath);
        if(rc==131 || rc==130 || stopped) break;
        const char *status="capture_failed";
        WavData wave={0}; AsrResult result={0}; FILE *file=NULL;
        post=now();
        if(rc) {fprintf(stderr,"Recording failed (%d).\n",rc);goto round_done;}
        status="wav_failed";
        file=fopen(wavpath,"rb");
        if(!file) {perror("Read recording");goto round_done;}
        rc=wav_read(file,&wave,error,sizeof(error)); fclose(file);file=NULL;
        if(rc) {fprintf(stderr,"%s\n",error);goto round_done;}
        duration=(double)wave.count/wave.sample_rate;
        status="recognition_failed";
        fprintf(stderr,"TRANSCRIBING...\n");
        if(asr_engine_transcribe(engine,&wave,&result,error,sizeof(error))) {
            fprintf(stderr,"%s\n",error);goto round_done;
        }
        decode=result.decode_seconds;
        status="text_failed";
        file=fopen(txtpath,"wx");
        if(!file) {perror("Create text");goto round_done;}
        rc=fprintf(file,"%s\n",result.text)<0;
        if(fclose(file)) rc=1;
        file=NULL;
        if(rc) {unlink(txtpath);fprintf(stderr,"Text write failed; WAV retained.\n");goto round_done;}
        status="ok";
        printf("%s\nAudio: %s\nText: %s\n",result.text,wavpath,txtpath);fflush(stdout);
round_done:
        post_end=now();
        round_end=post_end-begin;
        fprintf(stderr,"round=%u status=%s audio_seconds=%.6f decode_seconds=%.6f post_capture_seconds=%.6f round_seconds=%.6f\n",
                round,status,duration,decode,post_end-post,round_end);
        fprintf(log,"%u\t%s\t%.6f\t%.6f\t%.6f\t%.6f\n",round,status,duration,decode,post_end-post,round_end);
        fprintf(csv,"%u,%s,%.6f,%.6f,%.6f,%.6f,%s,%s\n",round,status,duration,decode,post_end-post,round_end,wavpath,txtpath);
        if(fflush(log) || ferror(log)) fprintf(stderr,"WARNING: timing log write failed.\n");
        if(fflush(csv) || ferror(csv)) fprintf(stderr,"WARNING: summary write failed.\n");
        wav_free(&wave);asr_result_free(&result);
        if(strcmp(status,"ok") && !stopped) {
            fprintf(stderr,"Press Enter to retry, q + Enter to quit.\n");
            char ch; int quit=0; ssize_t n;
            do {n=read(STDIN_FILENO,&ch,1);if(n>0 && ch=='q')quit=1;} while(n>0 && ch!='\n' && !stopped);
            if(n<=0 || quit) break;
        }
    }
    asr_engine_destroy(engine);fclose(csv);fclose(log);
    return stopped?130:0;
}
