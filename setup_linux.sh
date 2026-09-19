#!/bin/sh
# 只下载到项目目录；不会安装系统软件。首次下载约188MB。
set -eu
cd "$(dirname "$0")"
if [ "$(uname -s)" != Linux ] || [ "$(uname -m)" != x86_64 ]; then
    echo 'This setup script currently supports Linux x86_64 only.' >&2
    exit 1
fi
for tool in curl sha256sum tar; do
    command -v "$tool" >/dev/null || { echo "Missing tool: $tool" >&2; exit 1; }
done
mkdir -p downloads deps models results recordings
fetch() {
    url=$1; archive=$2; checksum=$3
    if [ ! -f "downloads/$archive" ]; then
        curl --fail --location --retry 2 "$url" -o "downloads/$archive.part"
        printf '%s  %s\n' "$checksum" "downloads/$archive.part" | sha256sum -c -
        mv "downloads/$archive.part" "downloads/$archive"
    fi
    printf '%s  %s\n' "$checksum" "downloads/$archive" | sha256sum -c -
}
runtime=sherpa-onnx-v1.13.8-linux-x64-shared-no-tts
model=sherpa-onnx-sense-voice-zh-en-ja-ko-yue-int8-2024-07-17
fetch "https://github.com/k2-fsa/sherpa-onnx/releases/download/v1.13.8/$runtime.tar.bz2" "$runtime.tar.bz2" d0f96c8b65c6cd0974fada22737e337de81bc8cd2abbec2e39caf358b1eec5fc
fetch "https://github.com/k2-fsa/sherpa-onnx/releases/download/asr-models/$model.tar.bz2" "$model.tar.bz2" 7d1efa2138a65b0b488df37f8b89e3d91a60676e416f515b952358d83dfd347e
if [ ! -f "deps/$runtime/include/sherpa-onnx/c-api/c-api.h" ] || [ ! -f "deps/$runtime/lib/libsherpa-onnx-c-api.so" ]; then
    tar -xjf "downloads/$runtime.tar.bz2" -C deps
fi
if [ ! -f "models/$model/model.int8.onnx" ] || [ ! -f "models/$model/tokens.txt" ] || [ ! -f "models/$model/test_wavs/zh.wav" ]; then
    tar -xjf "downloads/$model.tar.bz2" -C models
fi
echo 'Dependencies ready. Next: sh build_linux.sh'
