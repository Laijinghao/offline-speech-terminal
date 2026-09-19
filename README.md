# 便携式离线语音转写终端 · C / Embedded Linux

应用源代码全部是 C，底层通过 sherpa-onnx 的正式 C API 使用预训练 SenseVoice 模型，不需要训练模型或编写 Python。

当前仅做本地 WAV 文件识别，不包含实时录音、分段、屏幕或 PCB。

## 从 GitHub 开始（Ubuntu x86_64）

准备 GCC、CMake（3.16及以上）、Make、Git、curl、tar/bzip2。可通过 Ubuntu 软件仓库安装这些工具。下载脚本只写入当前项目，不自动安装系统软件。

```sh
git clone https://github.com/Laijinghao/offline-speech-terminal.git
cd offline-speech-terminal
sh setup_linux.sh
sh build_linux.sh
sh run_sample.sh
```

首次下载约188MB，模型和推理库均取自官方GitHub发布页，并校验固定SHA256。首次安装需要联网，识别过程不需要联网。依赖与权重许可证见 [DEPENDENCIES.md](docs/DEPENDENCIES.md)。

## 已配置的本地 Ubuntu 中立即运行

本次开发机器的项目已放在 `~/offline_asr`。打开 Ubuntu 终端：

```sh
cd ~/offline_asr
sh run_sample.sh
```

重新编译和测试：`sh build_linux.sh`。不需要输入 sudo 密码。

详细实测数据、环境与尚未完成事项见 `docs/VALIDATION.md`。

## 输入与输出

- WAV：16kHz、单声道、16位整数 PCM，时长大于0且不超过60秒。
- 模型目录：`model.int8.onnx` 和 `tokens.txt`。
- 输出：UTF-8 中文；诊断及模型加载/解码耗时写入标准错误。
- `--output` 可选，文件必须尚不存在，以免覆盖输入或既有结果。
- 默认使用2个CPU线程；`--threads` 可选1至8。

## Ubuntu 构建

需要 GCC、CMake、Make；在项目根目录执行：

```sh
cmake -S . -B build -DSHERPA_ROOT="$PWD/deps/sherpa-onnx-v1.13.8-linux-x64-shared-no-tts"
cmake --build build -j2
(cd build && ctest --output-on-failure)
./build/asr_demo --model-dir ./models/sherpa-onnx-sense-voice-zh-en-ja-ko-yue-int8-2024-07-17 --wav ./models/sherpa-onnx-sense-voice-zh-en-ja-ko-yue-int8-2024-07-17/test_wavs/zh.wav --output ./results/zh.txt
```

运行前确保依赖与模型已按对应目录解压，`results` 目录存在。再次运行时换一个输出文件名，或者省略 `--output`。

本次开发环境原先没有 CMake/Make，额外工具解压在本地 `tools/`，没有替换系统库；`build_linux.sh` 会只在当前进程中设置所需路径。这个目录不上传GitHub，新克隆项目应自行准备系统构建工具。

## 换成自己的录音

把一段符合格式的录音放在项目目录，例如 `my_voice.wav`，然后运行：

```sh
./build/asr_demo --model-dir ./models/sherpa-onnx-sense-voice-zh-en-ja-ko-yue-int8-2024-07-17 --wav ./my_voice.wav --output ./results/my_voice.txt
```

手机录音常见的 M4A 不能直接改后缀当 WAV；应使用音频工具转换成 16000Hz、单声道、PCM16 后再输入。目前不自动启动麦克风，也不采集个人录音。

## Windows 辅助验证

在 PowerShell 中进入 Windows 项目的 `offline_asr` 目录，可运行 `build/asr_demo.exe`，参数同上。Ubuntu 是主要验收环境。Windows 的编译版本只方便对照，不表示开发板已经部署。

本机 Windows 已有 MinGW GCC，可运行 `./build_windows.ps1` 重新编译和测试。Windows 与 Ubuntu 的 build 目录分别保留，不能互相复制使用。

第一阶段仅支持 x86_64 主机发行包；以后在 ARM 开发板上必须更换匹配架构的依赖并重新编译。

## 先读哪些代码

1. `src/main.c`：参数、文件检查、调用顺序、保存结果及退出码。
2. `src/wav.c`：RIFF/WAV 分块读取，把有符号 PCM16 转为浮点采样。
3. `src/asr.c`：配置并调用识别库，管理资源和计时。
4. `tests/test_wav.c`：构造有效/损坏的音频头进行测试，不依赖模型。

## 性能指标

`load_seconds` 是初始化模型耗时；`decode_seconds` 包含采样送入、解码和结果提取；`RTF` 是解码耗时除以录音时长。不包括读写文件和用户讲话等待，也不代表逐字实时字幕。

## 边界

- 不上传音频，不调用在线识别服务；依赖和模型需要提前下载。
- 官方短样例只用于验证流程，不能代表准确率。已测试两条个人普通话录音，结果见验证记录；尚未进行正式准确率统计。
- 格式不合要求时先用现有音频工具转换；不自行实现MP3/M4A解码。
- 模型必须来自可信官方来源。损坏的模型可能被底层运行库直接终止；本程序不能保证捕获底层C++库的所有致命错误。
- Windows 验证建议在项目目录中使用英文相对文件名；第一版未实现Windows Unicode命令行路径转换，任意中文文件名兼容性不作保证。UTF-8识别文字与Linux路径不受此限制。
- 退出码：0成功、2参数问题、3输入/模型文件问题、4输出问题、5识别初始化或结果问题。

项目需求见 [PROJECT_SCOPE.md](docs/PROJECT_SCOPE.md)，开发过程见 [开发日志](docs/DEVELOPMENT_LOG.md)，实测与限制见 [验证记录](docs/VALIDATION.md)。

仓库仅包含源码、脚本、测试与文档，不包含个人录音、原始个人测试输出、模型、依赖二进制和构建产物。
