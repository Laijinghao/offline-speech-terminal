#!/bin/sh
# Enter to start, Enter to stop, then transcribe one sentence.
set -eu
cd "$(dirname "$0")"
if [ "$#" -gt 1 ]; then echo 'Usage: sh run_sentence.sh [ALSA device]' >&2; exit 2; fi
mkdir -p recordings results
session="sentence_$(date +%Y%m%d_%H%M%S)_$$"
wav="recordings/$session.wav"
text="results/$session.txt"
./build/record_wav --interactive --device "${1:-default}" --output "$wav"
printf '\nTRANSCRIBING...\n'
./build/asr_demo --model-dir ./models/sherpa-onnx-sense-voice-zh-en-ja-ko-yue-int8-2024-07-17 --wav "$wav" --output "$text"
printf '\nAudio: %s\nText: %s\n' "$wav" "$text"
