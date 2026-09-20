"""Linux PTY integration tests; synthetic ALSA input, never real microphone."""
import os
import pty
import select
import signal
import subprocess
import sys
import tempfile
import time
import threading
import wave
from pathlib import Path

recorder, fixture, config = map(os.path.abspath, sys.argv[1:])
with tempfile.TemporaryDirectory(prefix='asr-interactive-') as directory:
    root = Path(directory)
    raw = root / 'input.raw'
    subprocess.run([fixture, 'generate', str(raw)], check=True)
    env = dict(os.environ, ASR_TEST_PCM=str(raw), ALSA_CONFIG_PATH=config)
    for cancel in (False, True):
        master, slave = pty.openpty()
        dest = root / ('cancel.wav' if cancel else 'capture.wav')
        process = subprocess.Popen([recorder, '--interactive', '--seconds', '1',
                                    '--device', 'test_capture', '--output', str(dest)],
                                   stdin=slave, stdout=slave, stderr=slave, env=env)
        os.close(slave)
        try:
            log = b''
            deadline = time.monotonic() + 5
            while b'READY:' not in log:
                if time.monotonic() > deadline:
                    raise AssertionError('READY timeout: ' + repr(log))
                if select.select([master], [], [], .1)[0]:
                    log += os.read(master, 4096)
            if cancel:
                process.send_signal(signal.SIGINT)
            else:
                os.write(master, b'\n')
            assert process.wait(timeout=5) == (130 if cancel else 0)
            if cancel:
                assert not dest.exists()
            else:
                subprocess.run([fixture, 'verify', str(dest)], check=True)
            assert not list(root.glob('*.tmp.*'))
        finally:
            if process.poll() is None:
                process.kill()
                process.wait()
            os.close(master)
    result = subprocess.run([recorder, '--interactive', '--output', str(root/'no-tty.wav')],
                            stdin=subprocess.DEVNULL, capture_output=True)
    assert result.returncode == 2
    assert not (root/'no-tty.wav').exists()
    # A paced FIFO supplies synthetic PCM so Enter can stop before the limit.
    fifo = root / 'paced.raw'
    os.mkfifo(fifo)
    def produce():
        try:
            with open(fifo, 'wb', buffering=0) as stream:
                for _ in range(80):
                    stream.write(b'\x01\x00' * 1024)
                    time.sleep(.064)
        except BrokenPipeError:
            pass
    writer = threading.Thread(target=produce, daemon=True)
    writer.start()
    master, slave = pty.openpty()
    output = root / 'stopped.wav'
    process = subprocess.Popen([recorder, '--interactive', '--seconds', '5',
                                '--device', 'test_capture', '--output', str(output)],
                               stdin=slave, stdout=slave, stderr=slave,
                               env=dict(env, ASR_TEST_PCM=str(fifo)))
    os.close(slave)
    try:
        def until(marker):
            data = b''
            deadline = time.monotonic() + 5
            while marker not in data:
                if time.monotonic() > deadline:
                    raise AssertionError('Prompt timeout: ' + repr(data))
                if select.select([master], [], [], .1)[0]:
                    data += os.read(master, 4096)
        until(b'READY:')
        os.write(master, b'\n')
        until(b'RECORDING:')
        time.sleep(.4)
        os.write(master, b'\n')
        assert process.wait(timeout=5) == 0
        with wave.open(str(output), 'rb') as wav:
            assert 0 < wav.getnframes() < 80000
            assert wav.getframerate() == 16000
        assert not list(root.glob('*.tmp.*'))
    finally:
        if process.poll() is None:
            process.kill()
            process.wait()
        os.close(master)
        writer.join(timeout=6)
print('Interactive start, limit, cancellation, non-terminal rejection: PASS')
print('Second Enter stops early and saves actual sample count: PASS')
