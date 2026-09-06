param([string]$Zig='zig',[string]$Python='python')
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
if (!(Test-Path (Join-Path $root 'assets\font.h'))) {
    & $Python (Join-Path $root 'tools\build_font.py')
    if ($LASTEXITCODE -ne 0) {throw 'Font generation failed'}
}
New-Item -ItemType Directory -Force (Join-Path $root 'build') | Out-Null
& $Zig c++ -target mipsel-linux-musleabi -mcpu=mips32r2 -std=c++17 -Oz -static -s -fno-rtti -Wall -Wextra (Join-Path $root 'src\main.cpp') -o (Join-Path $root 'build\c1sudoku')
if ($LASTEXITCODE -ne 0) {throw 'Compile failed'}
& $Python (Join-Path $root 'tools\build_integration.py')
if ($LASTEXITCODE -ne 0) {throw 'Integration failed'}
