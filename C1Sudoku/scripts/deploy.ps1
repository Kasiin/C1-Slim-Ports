param([string]$Adb='adb')
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$artifacts=Join-Path $root 'build\integration'
$devices=@(& $Adb devices | Select-Object -Skip 1 | Where-Object {$_ -match "\tdevice$"})
if ($devices.Count -ne 1) {throw 'Expected exactly one connected ADB device'}
& $Adb shell 'mkdir -p /usr/data/c1sudoku-stage'
if ($LASTEXITCODE -ne 0) {throw 'Stage creation failed'}
& $Adb push "$artifacts\." /usr/data/c1sudoku-stage
if ($LASTEXITCODE -ne 0) {throw 'Upload failed'}
& $Adb shell 'chmod 755 /usr/data/c1sudoku-stage/install.sh && /bin/sh -n /usr/data/c1sudoku-stage/launch-sudoku.sh && /usr/data/c1sudoku-stage/install.sh'
if ($LASTEXITCODE -ne 0) {throw 'Installation failed; inspect recovery state'}
& $Adb shell '/storage/c1sudoku/c1sudoku --selftest && sha256sum /usr/bin/d261/mpenMain /usr/data/s /storage/c1sudoku/c1sudoku; pidof mpenMain'
if ($LASTEXITCODE -ne 0) {throw 'Post-install check failed'}
