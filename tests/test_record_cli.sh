#!/bin/sh
set -eu
exe=$1
expect() {
    expected=$1; shift
    actual=0
    "$exe" "$@" >/dev/null 2>&1 || actual=$?
    if [ "$actual" -ne "$expected" ]; then
        printf 'FAIL expected %s got %s\n' "$expected" "$actual"; exit 1
    fi
}
expect 2
expect 2 --seconds 0 --output /tmp/invalid.wav
expect 2 --seconds 61 --output /tmp/invalid.wav
expect 2 --seconds abc --output /tmp/invalid.wav
expect 2 --seconds 1 --seconds 2 --output /tmp/invalid.wav
expect 2 --unknown yes
expect 4 --output /dev/null
expect 3 --device codex_asr_nonexistent_device --seconds 1 --output /tmp/record_test_nonexistent.wav
expect 0 --help
printf 'Record CLI tests: PASS (no microphone capture)\n'
