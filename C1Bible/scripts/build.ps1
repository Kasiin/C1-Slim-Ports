param([string]$Zig='zig',[string]$Python='python')
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
& $Python -c 'import PIL, fontTools, pypinyin'
if ($LASTEXITCODE -ne 0) {throw 'Install Python dependencies: python -m pip install -r requirements.txt'}
& $Python (Join-Path $root 'tools\build_data.py')
if ($LASTEXITCODE -ne 0) {throw 'Bible data generation failed'}
& $Python (Join-Path $root 'tools\build_font.py')
if ($LASTEXITCODE -ne 0) {throw 'Bible font generation failed'}
New-Item -ItemType Directory -Force (Join-Path $root 'build') | Out-Null
& $Zig c++ -target mipsel-linux-musleabi -mcpu=mips32r2 -std=c++17 -Oz -static -s -fno-rtti -Wall -Wextra (Join-Path $root 'src\main.cpp') -o (Join-Path $root 'build\c1bible')
if ($LASTEXITCODE -ne 0) {throw 'Compile failed'}
& $Python (Join-Path $root 'tools\build_integration.py')
if ($LASTEXITCODE -ne 0) {throw 'Integration failed'}
