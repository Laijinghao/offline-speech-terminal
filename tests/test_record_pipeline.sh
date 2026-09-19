#!/bin/sh
set -eu
recorder=$1
fixture=$2
config=$3
tmp=$(mktemp -d)
cleanup() {
    rm -f "$tmp/input.raw" "$tmp/captured.wav" "$tmp/record.log" "$tmp/cancel.log"
    rmdir "$tmp"
}
trap cleanup EXIT
"$fixture" generate "$tmp/input.raw"
ASR_TEST_PCM="$tmp/input.raw" ALSA_CONFIG_PATH="$config" "$recorder" --device test_capture --seconds 1 --output "$tmp/captured.wav" >"$tmp/record.log" 2>&1 || { cat "$tmp/record.log"; exit 1; }
"$fixture" verify "$tmp/captured.wav"
# 在倒计时阶段发SIGINT，不访问真实麦克风，也不留下部分WAV。
ASR_TEST_PCM="$tmp/input.raw" ALSA_CONFIG_PATH="$config" "$recorder" --device test_capture --seconds 1 --output "$tmp/cancelled.wav" >"$tmp/cancel.log" 2>&1 &
pid=$!
sleep 1
kill -INT "$pid"
rc=0
wait "$pid" || rc=$?
test "$rc" -eq 130
test ! -e "$tmp/cancelled.wav"
test -z "$(find "$tmp" -name '*.tmp.*' -print)"
printf 'Cancellation: PASS, no partial output\n'
