# Update the app only: no launcher restart, no root remount, no cache deletion.
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$artifact=Join-Path $root 'build\c1news'
$expected=(Get-FileHash $artifact -Algorithm SHA256).Hash.ToLower()
$launcherExpected='d6e85e7cca580d534b0a467c7b5fba63a8d84b745de768538a6ca8e7fcf76385'
$oldApp='bb66850dd095d15cce8bf30d8af034bd22c264773a669d613abadcc645e37d93'
$devices=@(& adb devices | Select-Object -Skip 1 | Where-Object {$_ -match "\tdevice$"})
if ($devices.Count -ne 1) {throw 'Connect exactly one C1 Slim by USB'}
$launcher=((& adb shell 'sha256sum /usr/bin/d261/mpenMain') -split '\s+')[0]
if ($launcher -ne $launcherExpected) {throw 'Unexpected launcher; no changes made'}
$active=(& adb shell 'pidof c1news') -join ' '
if ($active -match '\d+') {throw 'Exit News before updating'}
$current=((& adb shell 'sha256sum /storage/c1news/c1news') -split '\s+')[0]
if ($current -ne $oldApp -and $current -ne $expected) {throw 'Unexpected app version; preserve it and inspect first'}
if ($current -eq $oldApp) {
 & adb shell 'mkdir -p /storage/c1/recovery/news; if [ ! -f /storage/c1/recovery/news/c1news.v1.1.0 ]; then cp /storage/c1news/c1news /storage/c1/recovery/news/c1news.v1.1.0; fi'
 if ($LASTEXITCODE -ne 0) {throw 'Backup failed'}
 $backup=((& adb shell 'sha256sum /storage/c1/recovery/news/c1news.v1.1.0') -split '\s+')[0]
 if ($backup -ne $oldApp) {throw 'Backup hash mismatch'}
}
& adb push $artifact /storage/c1news/c1news.new
if ($LASTEXITCODE -ne 0) {throw 'Upload failed'}
$actual=((& adb shell 'sha256sum /storage/c1news/c1news.new') -split '\s+')[0]
if ($actual -ne $expected) {throw 'Upload hash mismatch'}
& adb shell 'chmod 755 /storage/c1news/c1news.new && mv /storage/c1news/c1news.new /storage/c1news/c1news && /storage/c1news/c1news --version'
if ($LASTEXITCODE -ne 0) {throw 'App update failed'}
& adb push (Join-Path $root 'build\integration\manifest.env') /storage/c1news/manifest.env
if ($LASTEXITCODE -ne 0) {throw 'Manifest upload failed'}
& adb push (Join-Path $root 'README.md') /storage/c1news/README.md
if ($LASTEXITCODE -ne 0) {throw 'Readme upload failed'}
Write-Host 'App updated; launcher and caches preserved. Press R to fetch SSPAI public text.'
