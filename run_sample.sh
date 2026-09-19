#!/bin/sh
set -eu
cd "$(dirname "$0")"
model=./models/sherpa-onnx-sense-voice-zh-en-ja-ko-yue-int8-2024-07-17
exec ./build/asr_demo --model-dir "$model" --wav "$model/test_wavs/zh.wav"
