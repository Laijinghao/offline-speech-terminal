#include "wav.h"
#include <stdlib.h>
#include <string.h>

static uint16_t u16(const unsigned char *p) { return (uint16_t)((unsigned)p[0] | (unsigned)p[1]<<8); }
static uint32_t u32(const unsigned char *p) { return (uint32_t)u16(p) | (uint32_t)u16(p+2)<<16; }
static int fail(char *err, size_t cap, const char *message) {
    if (cap) snprintf(err,cap,"%s",message);
    return -1;
}
void wav_free(WavData *w) { free(w->samples); memset(w,0,sizeof(*w)); }

static void put16(unsigned char *p, uint16_t v) { p[0]=(unsigned char)v; p[1]=(unsigned char)(v>>8); }
static void put32(unsigned char *p, uint32_t v) { put16(p,(uint16_t)v); put16(p+2,(uint16_t)(v>>16)); }
int wav_write_pcm16(FILE *f, const int16_t *samples, int32_t count, char *err, size_t cap) {
    unsigned char header[44]={0};
    if (!f || !samples || count<=0 || count>960000)
        return fail(err,cap,"PCM16 output requires 1..960000 samples.");
    uint32_t bytes=(uint32_t)count*2;
    memcpy(header,"RIFF",4); put32(header+4,36+bytes);
    memcpy(header+8,"WAVEfmt ",8); put32(header+16,16); put16(header+20,1);
    put16(header+22,1); put32(header+24,16000); put32(header+28,32000);
    put16(header+32,2); put16(header+34,16); memcpy(header+36,"data",4); put32(header+40,bytes);
    if(fwrite(header,1,sizeof(header),f)!=sizeof(header)) return fail(err,cap,"WAV header write failed.");
    for(int32_t i=0;i<count;++i) {
        unsigned char p[2]; put16(p,(uint16_t)samples[i]);
        if(fwrite(p,1,2,f)!=2) return fail(err,cap,"WAV sample write failed.");
    }
    if(fflush(f)!=0) return fail(err,cap,"WAV flush failed.");
    return 0;
}

int wav_read(FILE *f, WavData *out, char *err, size_t cap) {
    unsigned char header[16], fmt[16];
    long file_size, data_pos=0;
    uint32_t data_bytes=0; int have_fmt=0, have_data=0;
    memset(out,0,sizeof(*out));
    if (fseek(f,0,SEEK_END)!=0 || (file_size=ftell(f))<12 || fseek(f,0,SEEK_SET)!=0)
        return fail(err,cap,"Cannot read WAV header (file too short or not seekable).");
    if (fread(header,1,12,f)!=12 || memcmp(header,"RIFF",4) || memcmp(header+8,"WAVE",4))
        return fail(err,cap,"Expected a RIFF/WAVE file, not MP3/M4A or raw audio.");
    uint64_t end=(uint64_t)u32(header+4)+8;
    if (end<12 || end>(uint64_t)file_size)
        return fail(err,cap,"Truncated or invalid RIFF length.");
    /* WAV 不一定只有固定的 44 字节头：遍历 fmt/data/JUNK 等分块。 */
    uint64_t pos=12;
    while (pos<end) {
        if (end-pos<8 || fseek(f,(long)pos,SEEK_SET)!=0 || fread(header,1,8,f)!=8)
            return fail(err,cap,"Truncated WAV chunk header.");
        uint32_t bytes=u32(header+4);
        uint64_t next=pos+8+(uint64_t)bytes+(bytes&1u);
        if (next>end) return fail(err,cap,"WAV chunk exceeds RIFF boundary.");
        if (!memcmp(header,"fmt ",4)) {
            if (have_fmt || bytes<16 || fread(fmt,1,16,f)!=16)
                return fail(err,cap,"Invalid or repeated fmt chunk.");
            if (u16(fmt)!=1 || u16(fmt+2)!=1 || u32(fmt+4)!=16000 || u16(fmt+14)!=16)
                return fail(err,cap,"Only PCM16, mono, 16000 Hz WAV is supported. Convert the audio first.");
            if (u16(fmt+12)!=2 || u32(fmt+8)!=32000)
                return fail(err,cap,"Invalid WAV block alignment or byte rate.");
            have_fmt=1;
        } else if (!memcmp(header,"data",4)) {
            if (have_data || bytes==0 || bytes%2 || bytes>16000u*2u*60u)
                return fail(err,cap,"Audio must contain 1..960000 PCM16 samples (maximum 60 seconds), in one data chunk.");
            have_data=1; data_pos=(long)(pos+8); data_bytes=bytes;
        }
        pos=next;
    }
    if (!have_fmt || !have_data) return fail(err,cap,"Missing fmt or data chunk.");
    out->samples=malloc((data_bytes/2)*sizeof(float));
    if (!out->samples) return fail(err,cap,"Not enough memory for audio.");
    if (fseek(f,data_pos,SEEK_SET)!=0) { wav_free(out); return fail(err,cap,"Cannot seek to audio data."); }
    for (uint32_t i=0; i<data_bytes/2; ++i) {
        unsigned char p[2];
        if (fread(p,1,2,f)!=2) { wav_free(out); return fail(err,cap,"Truncated audio samples."); }
        int value=u16(p); if (value>=32768) value-=65536;
        out->samples[i]=(float)value/32768.0f;
    }
    out->count=(int32_t)(data_bytes/2); out->sample_rate=16000;
    return 0;
}
