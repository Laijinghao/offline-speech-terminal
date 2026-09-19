#include "wav.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
static int16_t sample(int i) { return (int16_t)(((i%101)-50)*200); }
int main(int argc,char **argv) {
    if(argc!=3) return 2;
    if(!strcmp(argv[1],"generate")) {
        FILE *f=fopen(argv[2],"wbx"); if(!f)return 3;
        for(int i=0;i<16000;++i) {
            uint16_t v=(uint16_t)sample(i);
            unsigned char p[2]={(unsigned char)v,(unsigned char)(v>>8)};
            if(fwrite(p,1,2,f)!=2) {fclose(f);return 4;}
        }
        return fclose(f)==0?0:4;
    }
    if(!strcmp(argv[1],"verify")) {
        FILE *f=fopen(argv[2],"rb");if(!f)return 3;
        WavData w={0};char err[256];int rc=wav_read(f,&w,err,sizeof(err));fclose(f);
        if(rc || w.count!=16000) {wav_free(&w);return 5;}
        for(int i=0;i<16000;++i) if(w.samples[i]!=(float)sample(i)/32768.0f) {wav_free(&w);return 6;}
        wav_free(&w);puts("Synthetic capture: all 16000 samples match");return 0;
    }
    return 2;
}
