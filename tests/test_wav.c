#include "wav.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static int failures;
static void check(int ok, const char *name) {
    if (!ok) { fprintf(stderr, "FAIL: %s\n", name); ++failures; }
}
static void le16(unsigned char *p, unsigned n) { p[0]=(unsigned char)n; p[1]=(unsigned char)(n>>8); }
static void le32(unsigned char *p, unsigned n) { le16(p,n); le16(p+2,n>>16); }
static void fixture(unsigned char *p) {
    memset(p,0,48); memcpy(p,"RIFF",4); le32(p+4,40); memcpy(p+8,"WAVEfmt ",8);
    le32(p+16,16); le16(p+20,1); le16(p+22,1); le32(p+24,16000);
    le32(p+28,32000); le16(p+32,2); le16(p+34,16); memcpy(p+36,"data",4);
    le32(p+40,4); le16(p+44,32767); le16(p+46,32768);
}
static int load(unsigned char *p, size_t n, WavData *w, char *err) {
    FILE *f=tmpfile();
    if (!f) { perror("tmpfile"); exit(2); }
    if (fwrite(p,1,n,f)!=n) exit(2);
    rewind(f); int rc=wav_read(f,w,err,256); fclose(f); return rc;
}
int main(void) {
    unsigned char p[64]; WavData w={0}; char err[256];
    fixture(p); check(load(p,48,&w,err)==0,"valid PCM16");
    check(w.count==2 && w.sample_rate==16000,"sample metadata");
    if (w.samples) check(fabs(w.samples[0]-32767.0/32768)<0.00001 && w.samples[1]==-1,"signed PCM conversion");
    wav_free(&w);
    fixture(p); check(load(p,47,&w,err)!=0,"truncated RIFF");
    fixture(p); p[0]='X'; check(load(p,48,&w,err)!=0,"invalid signature");
    fixture(p); le16(p+22,2); check(load(p,48,&w,err)!=0,"stereo rejected");
    fixture(p); le32(p+24,8000); check(load(p,48,&w,err)!=0,"8k rejected");
    fixture(p); le16(p+20,3); check(load(p,48,&w,err)!=0,"float rejected");
    fixture(p); le32(p+40,0xffffffffu); check(load(p,48,&w,err)!=0,"chunk exceeds file");
    fixture(p); le32(p+40,3); check(load(p,48,&w,err)!=0,"odd PCM length");
    fixture(p); le32(p+40,0); check(load(p,48,&w,err)!=0,"empty audio");
    fixture(p); le16(p+32,4); check(load(p,48,&w,err)!=0,"invalid block alignment");
    fixture(p); le32(p+28,1); check(load(p,48,&w,err)!=0,"invalid byte rate");
    fixture(p); memmove(p+46,p+36,12); memcpy(p+36,"JUNK",4); le32(p+40,1); p[44]=1; p[45]=0; le32(p+4,50);
    check(load(p,58,&w,err)==0,"unknown odd chunk with padding"); wav_free(&w);
    fixture(p); memmove(p+24,p+12,24); memcpy(p+12,"data",4); le32(p+16,4); le16(p+20,0); le16(p+22,0);
    check(load(p,48,&w,err)==0,"data before fmt"); wav_free(&w);
    printf("WAV tests: %s\n", failures?"FAILED":"PASS"); return failures?1:0;
}
