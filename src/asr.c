#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif
#include "asr.h"
#include "sherpa-onnx/c-api/c-api.h"
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

/* 单调时钟不受修改系统时间影响；这里只为计时保留平台差异。 */
static double now_seconds(void) {
#ifdef _WIN32
    LARGE_INTEGER counter, frequency;
    QueryPerformanceCounter(&counter); QueryPerformanceFrequency(&frequency);
    return (double)counter.QuadPart/(double)frequency.QuadPart;
#else
    struct timespec t;
    if (clock_gettime(CLOCK_MONOTONIC,&t)!=0) return 0;
    return (double)t.tv_sec+(double)t.tv_nsec/1e9;
#endif
}
const char *asr_version(void) { return SherpaOnnxGetVersionStr(); }
void asr_result_free(AsrResult *r) { free(r->text); memset(r,0,sizeof(*r)); }
int asr_transcribe(const char *model, const char *tokens, int threads,
                   const WavData *wave, AsrResult *out, char *err, size_t cap) {
    const SherpaOnnxOfflineRecognizer *recognizer=NULL;
    const SherpaOnnxOfflineStream *stream=NULL;
    const SherpaOnnxOfflineRecognizerResult *result=NULL;
    const char *message="Recognizer initialization failed. Check model and tokens.";
    int status=-1;
    memset(out,0,sizeof(*out));
    /* C API 的结构体必须清零，再设置实际使用的 SenseVoice 字段。 */
    SherpaOnnxOfflineRecognizerConfig config;
    memset(&config,0,sizeof(config));
    config.feat_config.sample_rate=16000;
    config.feat_config.feature_dim=80;
    config.decoding_method="greedy_search";
    config.model_config.provider="cpu";
    config.model_config.num_threads=threads;
    config.model_config.tokens=tokens;
    config.model_config.sense_voice.model=model;
    config.model_config.sense_voice.language="zh";
    config.model_config.sense_voice.use_itn=1;
    double start=now_seconds();
    recognizer=SherpaOnnxCreateOfflineRecognizer(&config);
    out->load_seconds=now_seconds()-start;
    if (!recognizer) goto cleanup;
    message="Cannot create recognition stream.";
    stream=SherpaOnnxCreateOfflineStream(recognizer);
    if (!stream) goto cleanup;
    /* 解码计时包含输入送入和结果提取，不包含模型加载、读文件和写文件。 */
    start=now_seconds();
    SherpaOnnxAcceptWaveformOffline(stream,wave->sample_rate,wave->samples,wave->count);
    SherpaOnnxDecodeOfflineStream(recognizer,stream);
    result=SherpaOnnxGetOfflineStreamResult(stream);
    out->decode_seconds=now_seconds()-start;
    message="Recognition did not return a valid result.";
    if (!result || !result->text) goto cleanup;
    out->text=malloc(strlen(result->text)+1);
    message="Cannot allocate result text.";
    if (!out->text) goto cleanup;
    strcpy(out->text,result->text); /* 在释放库对象前复制结果；调用方负责释放。 */
    status=0;
cleanup:
    if (result) SherpaOnnxDestroyOfflineRecognizerResult(result);
    if (stream) SherpaOnnxDestroyOfflineStream(stream);
    if (recognizer) SherpaOnnxDestroyOfflineRecognizer(recognizer);
    if (status && cap) snprintf(err,cap,"%s",message);
    return status;
}
