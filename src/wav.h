#ifndef ASR_WAV_H
#define ASR_WAV_H
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
/* 模型输入：单声道 16kHz；samples 为归一化到 [-1,1) 的浮点采样。 */
typedef struct { float *samples; int32_t count; int32_t sample_rate; } WavData;
int wav_read(FILE *file, WavData *out, char *error, size_t error_size);
void wav_free(WavData *wave);
/* 写入标准 16kHz/mono/PCM16 WAV；文件由调用方打开和关闭。 */
int wav_write_pcm16(FILE *file, const int16_t *samples, int32_t count,
                    char *error, size_t error_size);
#endif
