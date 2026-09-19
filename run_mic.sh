#!/bin/sh
# sh run_mic.sh [seconds] [ALSA device]；先录音，保存成功后再识别。
set -eu
cd "$(dirname "$0")"
if [ "$#" -gt 2 ]; then echo 'Usage: sh run_mic.sh [1..60 seconds] [device]' >&2; exit 2; fi
seconds=${1:-10}
device=${2:-default}
mkdir -p recordings results
session="mic_$(date +%Y%m%d_%H%M%S)_$$"
wav="recordings/$session.wav"
text="results/$session.txt"
./build/record_wav --seconds "$seconds" --device "$device" --output "$wav"
./build/asr_demo --model-dir ./models/sherpa-onnx-sense-voice-zh-en-ja-ko-yue-int8-2024-07-17 --wav "$wav" --output "$text"
printf '\nAudio: %s\nText: %s\n' "$wav" "$text"
