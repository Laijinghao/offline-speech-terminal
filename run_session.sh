#!/bin/sh
set -eu
cd "$(dirname "$0")"
if [ "$#" -gt 1 ]; then echo 'Usage: sh run_session.sh [ALSA device]' >&2; exit 2; fi
mkdir -p recordings
exec ./build/asr_session ./models/sherpa-onnx-sense-voice-zh-en-ja-ko-yue-int8-2024-07-17 ./build/record_wav "${1:-default}" ./recordings
