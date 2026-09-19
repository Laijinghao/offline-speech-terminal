#include "asr.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <windows.h>
#endif

static void usage(void) {
    puts("Usage: asr_demo --model-dir DIR --wav FILE [--output NEW_FILE] [--threads 1..8]\n"
         "Input: mono 16000 Hz PCM16 WAV, 0 < duration <= 60 seconds.\n"
         "Model: SenseVoice model.int8.onnx + tokens.txt. Default: 2 CPU threads.\n"
         "Existing output files are never overwritten. Text is UTF-8.\n"
         "Exit codes: 0 success, 2 arguments, 3 input/model files, 4 output, 5 recognition.");
}
static char *join_path(const char *dir, const char *name) {
    size_t n=strlen(dir)+strlen(name)+2;
    char *p=malloc(n);
    if (p) snprintf(p,n,"%s/%s",dir,name);
    return p;
}
static int readable_file(const char *path) {
    struct stat s; FILE *f;
    if (stat(path,&s)!=0 || s.st_size<=0) return 0;
#ifdef _WIN32
    if ((s.st_mode&_S_IFMT)!=_S_IFREG) return 0;
#else
    if (!S_ISREG(s.st_mode)) return 0;
#endif
    f=fopen(path,"rb"); if (!f) return 0; fclose(f); return 1;
}
int main(int argc, char **argv) {
    const char *dir=NULL, *input=NULL, *output=NULL;
    char *model=NULL, *tokens=NULL, error[256];
    int threads=2, seen_threads=0, status=2;
    FILE *f=NULL; WavData wave={0}; AsrResult result={0};
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    for (int i=1; i<argc; ++i) {
        if (!strcmp(argv[i],"--help") && argc==2) { usage(); return 0; }
        if (i+1>=argc) { fprintf(stderr,"Missing argument value: %s\n",argv[i]); goto done; }
        if (!strcmp(argv[i],"--model-dir") && !dir) dir=argv[++i];
        else if (!strcmp(argv[i],"--wav") && !input) input=argv[++i];
        else if (!strcmp(argv[i],"--output") && !output) output=argv[++i];
        else if (!strcmp(argv[i],"--threads") && !seen_threads) {
            char *end; errno=0; long n=strtol(argv[++i],&end,10); seen_threads=1;
            if (errno || end==argv[i] || *end || n<1 || n>8) { fprintf(stderr,"threads must be 1..8\n"); goto done; }
            threads=(int)n;
        } else { fprintf(stderr,"Unknown or repeated argument: %s\n",argv[i]); goto done; }
    }
    if (!dir || !*dir || !input || !*input || (output && !*output)) { usage(); goto done; }
    status=3;
    if (!readable_file(input) || !(f=fopen(input,"rb"))) { fprintf(stderr,"Cannot read WAV: %s\n",input); goto done; }
    int rc=wav_read(f,&wave,error,sizeof(error)); fclose(f); f=NULL;
    if (rc) { fprintf(stderr,"%s\n",error); goto done; }
    model=join_path(dir,"model.int8.onnx"); tokens=join_path(dir,"tokens.txt");
    if (!model || !tokens || !readable_file(model) || !readable_file(tokens)) {
        fprintf(stderr,"Model directory must contain readable model.int8.onnx and tokens.txt: %s\n",dir); goto done;
    }
    /* 提前排除已存在的输出；最终用独占创建，避免覆盖录音、模型或其他文件。 */
    if (output) { struct stat s; if (stat(output,&s)==0) {
        fprintf(stderr,"Output already exists; choose a new filename: %s\n",output); status=4; goto done;
    } }
    status=5;
    fprintf(stderr,"sherpa-onnx=%s provider=cpu threads=%d language=zh itn=1\n",asr_version(),threads);
    if (asr_transcribe(model,tokens,threads,&wave,&result,error,sizeof(error))) { fprintf(stderr,"%s\n",error); goto done; }
    double duration=(double)wave.count/wave.sample_rate;
    fprintf(stderr,"audio_seconds=%.6f load_seconds=%.6f decode_seconds=%.6f RTF=%.6f\n",
            duration,result.load_seconds,result.decode_seconds,result.decode_seconds/duration);
    status=4;
    if (output) {
        f=fopen(output,"wbx");
        if (!f) { fprintf(stderr,"Cannot create output '%s': %s\n",output,strerror(errno)); goto done; }
        int write_ok=fprintf(f,"%s\n",result.text)>=0;
        if (fclose(f)!=0) write_ok=0;
        f=NULL;
        if (!write_ok) { fprintf(stderr,"Output write failed; file may be incomplete: %s\n",output); goto done; }
    }
    if (printf("%s\n",result.text)<0 || fflush(stdout)!=0) { fprintf(stderr,"Cannot write stdout.\n"); goto done; }
    status=0;
done:
    if (f) fclose(f);
    wav_free(&wave); asr_result_free(&result); free(model); free(tokens);
    return status;
}
