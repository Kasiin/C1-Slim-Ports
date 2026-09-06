$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$artifacts=Join-Path $root 'build\integration'
& adb shell 'mkdir -p /usr/data/c1news-stage'
if ($LASTEXITCODE -ne 0) {throw 'Stage creation failed'}
& adb push "$artifacts\." /usr/data/c1news-stage
if ($LASTEXITCODE -ne 0) {throw 'Upload failed'}
& adb shell '/bin/sh -n /usr/data/c1news-stage/launch-news.sh && /bin/sh /usr/data/c1news-stage/install.sh'
if ($LASTEXITCODE -ne 0) {throw 'Installation failed; inspect recovery state'}
& adb shell '/storage/c1news/c1news --version; sha256sum /usr/bin/d261/mpenMain /usr/data/d /storage/c1news/c1news; pidof mpenMain'
if ($LASTEXITCODE -ne 0) {throw 'Post-install check failed'}
