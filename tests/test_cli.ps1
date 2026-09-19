param([string]$Exe = '.\build\asr_demo.exe')
$ErrorActionPreference='Stop'
if (-not (Test-Path -LiteralPath $Exe)) { throw 'Application not built yet' }
$model='./models/sherpa-onnx-sense-voice-zh-en-ja-ko-yue-int8-2024-07-17'
function Expect-Failure([string]$Name,[string[]]$Arguments,[int]$Code) {
    & $Exe @Arguments 2>&1 | Out-Null
    if ($LASTEXITCODE -ne $Code) { throw "$Name returned $LASTEXITCODE, expected $Code" }
    Write-Output "PASS: $Name"
}
Expect-Failure 'missing arguments' @() 2
Expect-Failure 'unknown argument' @('--unknown') 2
Expect-Failure 'missing WAV' @('--model-dir',$model,'--wav','no-such-file.wav') 3
Expect-Failure 'invalid WAV' @('--model-dir',$model,'--wav','README.md') 3
Expect-Failure 'missing model' @('--model-dir','no-model','--wav',"$model/test_wavs/zh.wav") 3
Expect-Failure 'invalid threads' @('--model-dir',$model,'--wav',"$model/test_wavs/zh.wav",'--threads','0') 2
Expect-Failure 'existing output' @('--model-dir',$model,'--wav',"$model/test_wavs/zh.wav",'--output','README.md') 4
Expect-Failure 'unwritable output path' @('--model-dir',$model,'--wav',"$model/test_wavs/zh.wav",'--output','no-such-directory/out.txt') 4
exit 0
