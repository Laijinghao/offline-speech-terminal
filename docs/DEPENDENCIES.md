# 固定依赖与来源

核验日期：2026-09-19。当前应用使用 C API，不调用 Python，也不依赖在线推理服务。

## sherpa-onnx v1.13.8

- 官方发布：https://github.com/k2-fsa/sherpa-onnx/releases/tag/v1.13.8
- Linux x86_64 CPU：https://github.com/k2-fsa/sherpa-onnx/releases/download/v1.13.8/sherpa-onnx-v1.13.8-linux-x64-shared-no-tts.tar.bz2
- Windows x86_64 CPU：https://github.com/k2-fsa/sherpa-onnx/releases/download/v1.13.8/sherpa-onnx-v1.13.8-win-x64-shared-MT-Release-no-tts.tar.bz2
- Linux 压缩包 24,802,494 字节。
- Linux 本地 SHA256：`d0f96c8b65c6cd0974fada22737e337de81bc8cd2abbec2e39caf358b1eec5fc`
- 源码许可证 Apache-2.0：https://github.com/k2-fsa/sherpa-onnx/blob/v1.13.8/LICENSE
- 内含 ONNX Runtime；其许可证为 MIT：https://github.com/microsoft/onnxruntime/blob/main/LICENSE
- 官方 C 示例参考：https://github.com/k2-fsa/sherpa-onnx/blob/v1.13.8/c-api-examples/sense-voice-c-api.c

## SenseVoice INT8（2024-07-17）

- 官方转换包：https://github.com/k2-fsa/sherpa-onnx/releases/download/asr-models/sherpa-onnx-sense-voice-zh-en-ja-ko-yue-int8-2024-07-17.tar.bz2
- 压缩包 163,002,883 字节，含中文等短样例录音。
- 本地 SHA256：`7d1efa2138a65b0b488df37f8b89e3d91a60676e416f515b952358d83dfd347e`
- 模型包 README 指向上游：https://github.com/FunAudioLLM/SenseVoice
- 使用 `model.int8.onnx` 和 `tokens.txt`，中文语言提示 `zh`，ITN 开启。
- 本项目输入策略为 16kHz/mono/PCM16，最长60秒；这是应用限制，不是对模型全部能力的声明。
- 权重模型卡：https://huggingface.co/FunAudioLLM/SenseVoiceSmall ，标注 `license: other`，指向 FunASR Model Open Source License Agreement（当前 1.1）：https://github.com/modelscope/FunASR/blob/main/MODEL_LICENSE 。需保留模型名称、作者与来源，不将权重声称为 Apache-2.0 或 MIT。

这些 SHA256 是本机下载后计算的完整性记录，不能单独等同于发布方签名认证。模型权重许可应依据上游模型卡独立核验，不把推理库许可证套用到权重。正式分发依赖前应一并保留对应许可证与通知。
