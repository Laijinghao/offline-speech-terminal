#ifndef ASR_ENGINE_H
#define ASR_ENGINE_H
#include "wav.h"
typedef struct { char *text; double load_seconds; double decode_seconds; } AsrResult;
int asr_transcribe(const char *model, const char *tokens, int threads,
                   const WavData *wave, AsrResult *out, char *error, size_t error_size);
void asr_result_free(AsrResult *result);
const char *asr_version(void);
#endif
