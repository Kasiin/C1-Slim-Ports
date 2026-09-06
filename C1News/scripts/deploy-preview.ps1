$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
& adb shell 'pidof c1news'
if ($LASTEXITCODE -eq 0) {throw 'Exit C1News before updating'}
& adb shell 'mkdir -p /storage/c1news'
if ($LASTEXITCODE -ne 0) {throw 'Directory creation failed'}
foreach ($name in @('c1news','cacert.pem')) {
 $file=Join-Path $root "build\$name"
 $expected=(Get-FileHash $file -Algorithm SHA256).Hash.ToLower()
 & adb push $file "/storage/c1news/$name.new"
 if ($LASTEXITCODE -ne 0) {throw "Upload failed: $name"}
 $actual=((& adb shell "sha256sum /storage/c1news/$name.new") -split '\s+')[0]
 if ($actual -ne $expected) {throw "Hash mismatch: $name"}
 & adb shell "mv /storage/c1news/$name.new /storage/c1news/$name"
 if ($LASTEXITCODE -ne 0) {throw "Install failed: $name"}
}
& adb push (Join-Path $root 'scripts\launch-news.sh') /storage/c1news/launch-news.sh.new
if ($LASTEXITCODE -ne 0) {throw 'Wrapper upload failed'}
& adb shell '/bin/sh -n /storage/c1news/launch-news.sh.new && mv /storage/c1news/launch-news.sh.new /storage/c1news/launch-news.sh && chmod 755 /storage/c1news/c1news /storage/c1news/launch-news.sh && /storage/c1news/c1news --version'
if ($LASTEXITCODE -ne 0) {throw 'Wrapper installation failed'}
Write-Host 'Preview installed; stock launcher and existing apps unchanged.'
