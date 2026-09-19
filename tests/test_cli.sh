#!/bin/sh
# 在项目根目录运行：sh tests/test_cli.sh [./build/asr_demo]
set -eu
exe=${1:-./build/asr_demo}
model=./models/sherpa-onnx-sense-voice-zh-en-ja-ko-yue-int8-2024-07-17
wav=$model/test_wavs/zh.wav
expect() {
    expected=$1; label=$2; shift 2
    actual=0
    "$exe" "$@" >/dev/null 2>&1 || actual=$?
    if [ "$actual" -ne "$expected" ]; then
        printf 'FAIL: %s: expected %s, actual %s\n' "$label" "$expected" "$actual"
        exit 1
    fi
    printf 'PASS: %s\n' "$label"
}
expect 2 'missing arguments'
expect 2 'unknown argument' --unknown
expect 3 'missing WAV' --model-dir "$model" --wav no-such-file.wav
expect 3 'invalid WAV' --model-dir "$model" --wav README.md
expect 3 'missing model' --model-dir no-model --wav "$wav"
expect 2 'invalid threads' --model-dir "$model" --wav "$wav" --threads 0
expect 4 'existing output' --model-dir "$model" --wav "$wav" --output README.md
expect 4 'unwritable output path' --model-dir "$model" --wav "$wav" --output no-such-directory/out.txt
