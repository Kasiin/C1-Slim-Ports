param([string]$Adb = 'adb')

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = Split-Path -Parent $PSScriptRoot
$workspace = Split-Path -Parent $root
$artifacts = Join-Path $root 'build\integration'

$devices = @(& $Adb devices | Select-Object -Skip 1 | Where-Object { $_ -match "\tdevice$" })
if ($devices.Count -ne 1) { throw 'Expected exactly one connected ADB device' }

& (Join-Path $workspace 'C1LavaX\port\scripts\build-c1lavax.ps1')
if ($LASTEXITCODE -ne 0) { throw 'C1LavaX build failed' }
& python (Join-Path $root 'tools\build_integration.py')
if ($LASTEXITCODE -ne 0) { throw 'Integration build failed' }

& $Adb shell 'mkdir -p /usr/data/c1mota-stage'
if ($LASTEXITCODE -ne 0) { throw 'Stage creation failed' }
& $Adb push "$artifacts\." /usr/data/c1mota-stage
if ($LASTEXITCODE -ne 0) { throw 'Upload failed' }
& $Adb shell 'chmod 755 /usr/data/c1mota-stage/install.sh && /bin/sh -n /usr/data/c1mota-stage/launch-mota.sh && /bin/sh -n /usr/data/c1mota-stage/install.sh && /usr/data/c1mota-stage/install.sh'
if ($LASTEXITCODE -ne 0) { throw 'Installation failed; inspect recovery state' }

& $Adb shell '/storage/c1mota/c1lavax --version; sha256sum /usr/bin/d261/mpenMain /usr/bin/d261/assets/images/ic_desktop_zjcs.png /usr/data/m /storage/c1mota/c1lavax /storage/c1mota/os/Mota.lav /storage/c1mota/os/LavaData/MOTA.dat; pidof mpenMain; grep -F "Automatic launcher update checks are disabled" /storage/c1mota/update-policy.txt'
if ($LASTEXITCODE -ne 0) { throw 'Post-install verification failed' }
