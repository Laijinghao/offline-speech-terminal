$ErrorActionPreference='Stop'
Set-Location $PSScriptRoot
$dep='deps/sherpa-onnx-v1.13.8-win-x64-shared-MT-Release-no-tts'
New-Item -ItemType Directory -Path build -Force | Out-Null
& gcc -std=c11 -Wall -Wextra -Wpedantic -O2 -I src -I "$dep/include" src/main.c src/wav.c src/asr.c "$dep/lib/sherpa-onnx-c-api.dll" -o build/asr_demo.exe
if ($LASTEXITCODE -ne 0) { throw 'C compilation failed' }
Copy-Item -Path "$dep/lib/*.dll" -Destination build
& gcc -std=c11 -Wall -Wextra -Wpedantic -I src tests/test_wav.c src/wav.c -o build/test_wav.exe
if ($LASTEXITCODE -ne 0) { throw 'Test compilation failed' }
& .\build\test_wav.exe
if ($LASTEXITCODE -ne 0) { throw 'WAV test failed' }
& .\tests\test_cli.ps1
