param([string]$Go='go',[string]$Python='python')
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
if (!(Test-Path (Join-Path $root 'font15.bin'))) {
 & $Python (Join-Path $root 'tools\build_font.py')
 if ($LASTEXITCODE -ne 0) {throw 'Font failed'}
}
New-Item -ItemType Directory -Force (Join-Path $root 'build') | Out-Null
Push-Location $root
try {
 $env:GOOS='windows';$env:GOARCH='amd64';$env:CGO_ENABLED='0'
 & $Go mod tidy
 if ($LASTEXITCODE -ne 0) {throw 'Dependencies failed'}
 & $Go test ./...
 if ($LASTEXITCODE -ne 0) {throw 'Tests failed'}
 & $Go build -trimpath -o build/c1news.exe .
 if ($LASTEXITCODE -ne 0) {throw 'Host build failed'}
 $env:GOOS='linux';$env:GOARCH='mipsle';$env:GOMIPS='hardfloat'
 & $Go build -trimpath -ldflags '-s -w -buildid=' -o build/c1news .
 if ($LASTEXITCODE -ne 0) {throw 'Device build failed'}
} finally {Pop-Location;Remove-Item Env:GOOS,Env:GOARCH,Env:CGO_ENABLED,Env:GOMIPS -ErrorAction SilentlyContinue}
