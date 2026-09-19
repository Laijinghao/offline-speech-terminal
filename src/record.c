#define _POSIX_C_SOURCE 200809L
#include <alsa/asoundlib.h>
#include "wav.h"
#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

static volatile sig_atomic_t stopped;
static void on_signal(int signo) { (void)signo; stopped=1; }
static double now(void) {
    struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t);
    return (double)t.tv_sec+(double)t.tv_nsec/1e9;
}
static void usage(void) {
    puts("record_wav --output NEW.wav [--seconds 1..60] [--device NAME]\n"
         "record_wav --list-devices\n"
         "Defaults: 10 seconds, default ALSA device; 3-second countdown.\n"
         "Writes mono 16000Hz PCM16. Ctrl+C cancels without publishing partial audio.\n"
         "Exit: 0 success, 2 arguments, 3 audio device, 4 output, 130 cancelled.");
}
static int list_devices(void) {
    void **hints=NULL;
    int rc=snd_device_name_hint(-1,"pcm",&hints);
    if(rc<0) { fprintf(stderr,"List devices: %s\n",snd_strerror(rc)); return 3; }
    for(void **p=hints; p && *p; ++p) {
        char *name=snd_device_name_get_hint(*p,"NAME");
        char *io=snd_device_name_get_hint(*p,"IOID");
        char *desc=snd_device_name_get_hint(*p,"DESC");
        if(name && (!io || strcmp(io,"Output"))) printf("%s\n  %s\n",name,desc?desc:"");
        free(name); free(io); free(desc);
    }
    snd_device_name_free_hint(hints); return 0;
}
int main(int argc,char **argv) {
    const char *device="default", *output=NULL;
    int seconds=10, have_seconds=0, have_device=0, status=2;
    snd_pcm_t *pcm=NULL; int16_t *samples=NULL; FILE *file=NULL;
    char *temporary=NULL, error[256]; int temp_created=0;
    if(argc==2 && !strcmp(argv[1],"--help")) {usage(); return 0;}
    if(argc==2 && !strcmp(argv[1],"--list-devices")) return list_devices();
    for(int i=1;i<argc;++i) {
        if(i+1>=argc) {usage(); goto cleanup;}
        if(!strcmp(argv[i],"--output") && !output) output=argv[++i];
        else if(!strcmp(argv[i],"--device") && !have_device) {device=argv[++i];have_device=1;}
        else if(!strcmp(argv[i],"--seconds") && !have_seconds) {
            char *end; errno=0; long n=strtol(argv[++i],&end,10); have_seconds=1;
            if(errno || end==argv[i] || *end || n<1 || n>60) {usage();goto cleanup;}
            seconds=(int)n;
        } else {usage();goto cleanup;}
    }
    if(!output || !*output || !*device) {usage();goto cleanup;}
    struct stat st;
    if(lstat(output,&st)==0) {fprintf(stderr,"Output exists: %s\n",output);status=4;goto cleanup;}
    struct sigaction sa={0}; sa.sa_handler=on_signal; sigemptyset(&sa.sa_mask);
    sigaction(SIGINT,&sa,NULL); sigaction(SIGTERM,&sa,NULL);
    status=3;
    int rc=snd_pcm_open(&pcm,device,SND_PCM_STREAM_CAPTURE,SND_PCM_NONBLOCK);
    if(rc<0) {fprintf(stderr,"Cannot open capture device '%s': %s\n",device,snd_strerror(rc));goto cleanup;}
    rc=snd_pcm_set_params(pcm,SND_PCM_FORMAT_S16,SND_PCM_ACCESS_RW_INTERLEAVED,1,16000,1,100000);
    if(rc<0) {fprintf(stderr,"Cannot configure 16kHz mono PCM16: %s\n",snd_strerror(rc));goto cleanup;}
    size_t target=(size_t)seconds*16000;
    samples=calloc(target,sizeof(*samples));
    if(!samples) {fprintf(stderr,"Cannot allocate capture buffer.\n");goto cleanup;}
    /* 同目录临时文件+独占发布，已有录音不会被覆盖。只有完整录音才命名为目标文件。 */
    status=4;
    temporary=malloc(strlen(output)+12);
    if(!temporary) goto cleanup;
    sprintf(temporary,"%s.tmp.XXXXXX",output);
    int fd=mkstemp(temporary);
    if(fd<0) {perror("Cannot create output temporary file");goto cleanup;}
    temp_created=1; file=fdopen(fd,"wb");
    if(!file) {close(fd);perror("fdopen");goto cleanup;}
    fprintf(stderr,"Device: %s; duration: %d seconds. Get ready.\n",device,seconds);
    for(int n=3;n>0 && !stopped;--n) {fprintf(stderr,"%d...\n",n);sleep(1);}
    if(stopped) {status=130;goto cleanup;}
    status=3;
    rc=snd_pcm_start(pcm);
    if(rc<0) {fprintf(stderr,"Cannot start capture: %s\n",snd_strerror(rc));goto cleanup;}
    fprintf(stderr,"RECORDING: speak now.\n");
    size_t received=0; double deadline=now()+seconds+10;
    while(received<target && !stopped) {
        if(now()>deadline) {fprintf(stderr,"Capture timeout. Check device or VM audio routing.\n");goto cleanup;}
        snd_pcm_uframes_t want=(snd_pcm_uframes_t)(target-received);
        if(want>1024) want=1024;
        snd_pcm_sframes_t got=snd_pcm_readi(pcm,samples+received,want);
        if(got==-EINTR) continue;
        if(got==-EAGAIN || got==0) {
            rc=snd_pcm_wait(pcm,250);
            if(rc<0 && rc!=-EINTR) {fprintf(stderr,"Capture wait failed: %s\n",snd_strerror(rc));goto cleanup;}
            continue;
        }
        if(got<0) {fprintf(stderr,"Capture failed: %s. Recording cancelled to avoid an unreported audio gap.\n",snd_strerror((int)got));goto cleanup;}
        received+=(size_t)got;
    }
    snd_pcm_drop(pcm);
    if(stopped) {status=130;goto cleanup;}
    unsigned peak=0;
    for(size_t i=0;i<target;++i) {int v=samples[i];unsigned a=(unsigned)(v<0?-v:v);if(a>peak)peak=a;}
    fprintf(stderr,"Capture complete: samples=%zu peak=%u/32768\n",target,peak);
    if(peak<32) fprintf(stderr,"WARNING: very weak or silent input; check microphone/mute settings before trusting transcription.\n");
    status=4;
    if(wav_write_pcm16(file,samples,(int32_t)target,error,sizeof(error))) {fprintf(stderr,"%s\n",error);goto cleanup;}
    rc=fclose(file); file=NULL;
    if(rc!=0) {perror("WAV close failed");goto cleanup;}
    if(stopped) {status=130;goto cleanup;}
    if(link(temporary,output)!=0) {perror("Cannot publish WAV (destination must be new, on a filesystem supporting hard links)");goto cleanup;}
    printf("Saved: %s\n",output); status=0;
cleanup:
    if(pcm) {snd_pcm_drop(pcm);snd_pcm_close(pcm);}
    if(file) fclose(file);
    if(temp_created) unlink(temporary);
    free(temporary);free(samples);
    if(stopped && status!=0) {fprintf(stderr,"Recording cancelled.\n");return 130;}
    return status;
}
