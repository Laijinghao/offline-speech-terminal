#!/bin/sh
set -eu
cd "$(dirname "$0")"
# 如系统已有构建工具可直接使用；本机额外工具解压在项目内，不需要 sudo。
if [ -d "$PWD/tools/usr/bin" ]; then
    PATH="$PWD/tools/usr/bin:$PATH"
    LD_LIBRARY_PATH="$PWD/tools/usr/lib/x86_64-linux-gnu${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
    export PATH LD_LIBRARY_PATH
fi
cmake -S . -B build -DSHERPA_ROOT="$PWD/deps/sherpa-onnx-v1.13.8-linux-x64-shared-no-tts"
cmake --build build -j2
(cd build && ctest --output-on-failure)
