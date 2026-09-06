param(
    [string]$AdbPath = 'adb'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$portRoot = Split-Path -Parent $PSScriptRoot
$artifacts = Join-Path $portRoot 'build\rich-integration'
$builder = Join-Path $portRoot 'tools\build_rich_integration.py'

& (Join-Path $PSScriptRoot 'build-c1lavax.ps1')
if ($LASTEXITCODE -ne 0) {
    throw 'C1LavaX build failed.'
}
python $builder
if ($LASTEXITCODE -ne 0) {
    throw 'Integration artifact build failed.'
}

$deviceLines = @(& $AdbPath devices | Select-Object -Skip 1 | Where-Object { $_ -match "\tdevice$" })
if ($deviceLines.Count -ne 1) {
    throw "Expected exactly one authorized ADB device; found $($deviceLines.Count)."
}

$stage = '/usr/data/wawa-rich-stage'
& $AdbPath shell "rm -rf '$stage' && mkdir -p '$stage'"
if ($LASTEXITCODE -ne 0) { throw 'Could not prepare the device staging directory.' }
& $AdbPath push "$(Join-Path $artifacts '.')" $stage
if ($LASTEXITCODE -ne 0) { throw 'Could not upload integration artifacts.' }
& $AdbPath shell "chmod 755 '$stage/install-rich.sh' '$stage/launch-rich.sh' && '$stage/install-rich.sh'"
if ($LASTEXITCODE -ne 0) { throw 'Device installation failed; the installer attempted rollback.' }
& $AdbPath shell "rm -rf '$stage'"
if ($LASTEXITCODE -ne 0) { throw 'Installation succeeded, but staging cleanup failed.' }

& $AdbPath shell "sha256sum /usr/bin/d261/mpenMain /usr/bin/d261/assets/images/ic_desktop_ccyj.png /storage/c1rich/c1lavax /usr/data/w; cat /storage/c1rich/install-state.txt; pidof mpenMain"
if ($LASTEXITCODE -ne 0) { throw 'Post-install verification failed.' }
