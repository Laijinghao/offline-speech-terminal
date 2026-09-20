"""Real model, simulated recorder: repeat inference and recover from bad input/output."""
import os
import pty
import select
import subprocess
import sys
import tempfile
import time
from pathlib import Path

binary, model, sample = map(os.path.abspath, sys.argv[1:])
with tempfile.TemporaryDirectory(prefix='asr-session-') as tmp:
    root = Path(tmp)
    recorder = root/'recorder.sh'
    recorder.write_text('''#!/bin/sh
for output; do :; done
case "$output" in
 *000002.wav) printf bad > "$output" ;;
 *000003.wav) cp "$SAMPLE" "$output"; printf sentinel > "${output%.wav}.txt" ;;
 *000005.wav) exit 3 ;;
 *000007.wav) exit 131 ;;
 *) cp "$SAMPLE" "$output" ;;
esac
''')
    recorder.chmod(0o700)
    master, slave = pty.openpty()
    process = subprocess.Popen([binary, model, str(recorder), 'fake', tmp],
                               stdin=slave, stdout=slave, stderr=slave,
                               env=dict(os.environ, SAMPLE=sample))
    os.close(slave)
    log = b''
    answered = 0
    deadline = time.monotonic()+60
    try:
        while process.poll() is None:
            assert time.monotonic()<deadline, log.decode(errors='replace')
            if select.select([master], [], [], .1)[0]:
                try:
                    data=os.read(master,65536)
                except OSError:
                    break
                log+=data
                prompts=log.count(b'Press Enter to retry')
                while answered<prompts:
                    os.write(master,b'\n');answered+=1
        assert process.wait(timeout=5)==0,log.decode(errors='replace')
        sessions=list(root.glob('session_*'))
        assert len(sessions)==1
        session=sessions[0]
        metrics=(session/'metrics.tsv').read_text()
        summary=(session/'summary.csv').read_text()
        assert metrics.count('model_load_seconds=')==1
        assert summary.splitlines()[0]=='round,status,audio_seconds,decode_seconds,post_capture_seconds,round_seconds,wav_file,text_file'
        summary_rows=summary.splitlines()[1:]
        assert len(summary_rows)==6
        assert summary_rows[0].startswith('1,ok,')
        assert summary_rows[-1].startswith('6,ok,')
        assert str(session/'round_000001.wav') in summary_rows[0]
        assert str(session/'round_000001.txt') in summary_rows[0]
        rows=[r.split('\t') for r in metrics.splitlines()[2:]]
        assert [r[1] for r in rows]==['ok','wav_failed','text_failed','ok','capture_failed','ok'],metrics
        assert (session/'round_000001.txt').read_text()==(session/'round_000004.txt').read_text()
        assert '开饭时间' in (session/'round_000004.txt').read_text()
        assert (session/'round_000003.txt').read_text()=='sentinel'
        assert not (session/'round_000002.txt').exists()
        assert (session/'round_000001.txt').read_text()==(session/'round_000006.txt').read_text()
        assert len(list(session.glob('*.wav')))==5
        print('PASS: one model load, repeated real inference, invalid WAV recovery, no overwrite, output failure recovery')
    finally:
        if process.poll() is None:
            process.kill();process.wait()
        os.close(master)
