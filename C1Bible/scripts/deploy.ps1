param([string]$Adb='adb')
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$artifacts=Join-Path $root 'build\integration'
$devices=@(& $Adb devices | Select-Object -Skip 1 | Where-Object {$_ -match "\tdevice$"})
if ($devices.Count -ne 1) {throw 'Expected exactly one connected ADB device'}
& $Adb shell 'rm -rf /storage/c1bible-stage && mkdir -p /storage/c1bible-stage'
if ($LASTEXITCODE -ne 0) {throw 'Stage creation failed'}
foreach ($file in Get-ChildItem -LiteralPath $artifacts -File) {
    $uploaded=$false
    for ($attempt=1; $attempt -le 3 -and -not $uploaded; $attempt++) {
        & $Adb push $file.FullName "/storage/c1bible-stage/$($file.Name)"
        $uploaded=($LASTEXITCODE -eq 0)
        if (-not $uploaded) {Start-Sleep -Seconds 1}
    }
    if (-not $uploaded) {throw "Upload failed: $($file.Name)"}
}
& $Adb shell 'chmod 755 /storage/c1bible-stage/install.sh && /bin/sh -n /storage/c1bible-stage/launch-bible.sh && /bin/sh /storage/c1bible-stage/install.sh'
if ($LASTEXITCODE -ne 0) {throw 'Installation failed; inspect recovery state'}
& $Adb shell 'set -e; /storage/c1bible/c1bible --selftest; sha256sum /usr/bin/d261/mpenMain /usr/data/h /storage/c1bible/c1bible /storage/c1bible/bible.dat; pidof mpenMain'
if ($LASTEXITCODE -ne 0) {throw 'Post-install check failed'}
