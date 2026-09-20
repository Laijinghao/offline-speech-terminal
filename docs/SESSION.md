# 模型常驻的多轮录音转写

启动：`sh run_session.sh [ALSA设备]`。默认 default，CPU 2 线程。

状态：加载模型一次 → 等待回车 → 录音 → 回车结束或达到 60 秒 → 新建独立识别流 → 保存结果 → 下一轮。等待开始时 q 加回车退出；Ctrl+C 结束会话。

复用已有 record_wav 采集和 WAV 解析；asr.c 分离创建、识别、释放接口，原 asr_transcribe 保留为单文件兼容封装。常驻 asr_session 父进程持有模型，每轮 fork/exec 录音子进程，随后用新的识别流处理一段音频并释放流。模型只在会话结束时释放。

每次启动通过 mkdtemp 创建独立 recordings/session_XXXXXX 目录，轮次编号递增。WAV 沿用独占发布，TXT 独占创建，已存在文件不会覆盖。录音和文字位于同一轮次路径；这些目录已由 Git 忽略。

metrics.tsv 首行记录首次 model_load_seconds；每轮记录状态、音频秒数、解码秒数、录音进程结束后的处理耗时及整轮耗时。summary.csv 使用逗号分隔，并额外记录本轮 WAV/TXT 路径，方便后续用表格软件或脚本统计。整轮耗时包含等待用户开始和讲话时间；post_capture_seconds 包含 WAV 读取、识别和 TXT 保存，不含模型首次加载。失败轮次中音频/解码的零值表示该步骤未完成。用户退出或取消不是已完成轮次，不计入表格。

录音失败、WAV 不合法、识别接口返回失败、TXT 创建或写入失败均提示并允许回车重试。成功采集的 WAV 在后续识别或保存失败时保留；文本写入失败只清理本轮新建的残缺 TXT。模型首次加载失败无法开始会话。底层推理库 abort/崩溃或操作系统 OOM 杀进程无法在同进程恢复；此处不声称可以恢复此类故障。

验证：原有回归测试；真实模型在同一进程内多轮识别；中间插入坏 WAV 后恢复；预置 TXT 哨兵验证不覆盖并在下一轮恢复；现有回车录音伪终端测试。模拟录音用于自动测试，不是本次真人多轮验收。Python 只用于测试驱动，产品应用仍为 C。

测试命令：

```sh
sh build_linux.sh
python3 tests/test_session.py build/asr_session models/sherpa-onnx-sense-voice-zh-en-ja-ko-yue-int8-2024-07-17 models/sherpa-onnx-sense-voice-zh-en-ja-ko-yue-int8-2024-07-17/test_wavs/zh.wav
python3 tests/test_record_interactive.py build/record_wav build/pcm_fixture tests/alsa_file.conf
```
