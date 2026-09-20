# 回车控制单句录音

用户确认的流程：回车开始 → 讲话 → 回车结束 → 保存 WAV → 调用现有 asr_demo → 显示并保存文字。

实现：复用 record_wav，新增 --interactive；run_sentence.sh 串联录音与识别。无需新增线程，录音循环通过 poll 检查终端输入。保持终端默认行输入方式，不修改终端属性。

最长录制 60 秒，达到上限自动保存。Ctrl+C 或终端输入关闭时取消，不进入识别；零采样不保存。保存实际采样数，不把未录到的缓冲区写入文件。现有定时录音模式继续保留。

运行方式（Linux 重新构建后）：

```sh
cd ~/offline_asr
sh build_linux.sh
sh run_sentence.sh
```

出现 READY 后按回车，出现 RECORDING 后讲话，再按回车。等待识别完成；下一句重新运行脚本。通过 SSH 使用时需要交互终端（ssh -t）。每句重新加载模型，目前不含 VAD、常驻模型或物理 GPIO 按键。

验证计划：原有 7 项回归；伪终端下的开始、自动截止及取消测试；用户实际耳机录音手动停止与转写。合成输入仅验证软件流程，不代替真实麦克风验收。

2026-09-20 验证结果：Ubuntu 构建成功，原有 7 项测试通过。新增伪终端测试通过：回车开始、上限自动停止、等待开始时取消、拒绝非终端输入、第二次回车提前停止及按实际采样数保存。测试通过限速 FIFO 提供合成 PCM，不使用真实麦克风。手动耳机录音验收待用户执行。

开发测试命令：`python3 tests/test_record_interactive.py build/record_wav build/pcm_fixture tests/alsa_file.conf`。Python 仅用于模拟终端的测试工具，录音应用仍为 C，实际运行无需 Python。
