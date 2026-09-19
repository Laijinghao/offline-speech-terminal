#include "wav.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

int main(void) {
    int16_t samples[]={-32768,-1,0,1,32767};
    char error[256]; WavData wave={0}; FILE *f=tmpfile();
    if (!f) return 1;
    if (wav_write_pcm16(f,samples,5,error,sizeof(error))!=0) return 2;
    rewind(f);
    if (wav_read(f,&wave,error,sizeof(error))!=0) return 3;
    if (wave.count!=5 || wave.sample_rate!=16000) return 4;
    for (int i=0;i<5;++i) if(wave.samples[i]!=(float)samples[i]/32768.0f) return 5;
    wav_free(&wave); fclose(f);
    f=tmpfile(); if(!f) return 6;
    if(wav_write_pcm16(f,samples,0,error,sizeof(error))==0) return 7;
    if(wav_write_pcm16(f,NULL,5,error,sizeof(error))==0) return 8;
    if(wav_write_pcm16(f,samples,960001,error,sizeof(error))==0) return 9;
    if(ftell(f)!=0) return 10; /* 无效参数不能写出半个文件头。 */
    fclose(f); puts("PCM16 write/read tests: PASS"); return 0;
}
