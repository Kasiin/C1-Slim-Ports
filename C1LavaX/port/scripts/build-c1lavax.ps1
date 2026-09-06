param(
    [string]$Output = "$(Split-Path -Parent $PSScriptRoot)\build\c1lavax",
    [string]$Zig = 'zig'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$portRoot = Split-Path -Parent $PSScriptRoot

$sources = @(
    (Join-Path $portRoot 'src\main.cpp')
) + @(
    Get-ChildItem -LiteralPath (Join-Path $portRoot 'third_party\lavax_vm') -Filter '*.cpp' |
        Sort-Object Name |
        ForEach-Object FullName
)

$outputPath = [IO.Path]::GetFullPath($Output)
$outputDirectory = Split-Path -Parent $outputPath
New-Item -ItemType Directory -Force -Path $outputDirectory | Out-Null

$arguments = @(
    'c++',
    '-target', 'mipsel-linux-musleabi',
    '-mcpu=mips32r2',
    '-std=c++17',
    '-Oz',
    '-static',
    '-s',
    '-fno-rtti',
    "-I$(Join-Path $portRoot 'third_party\lavax_vm')"
) + $sources + @('-o', $outputPath)

& $Zig @arguments
if ($LASTEXITCODE -ne 0) {
    throw "C1LavaX cross-build failed with exit code $LASTEXITCODE"
}

$hash = (Get-FileHash -LiteralPath $outputPath -Algorithm SHA256).Hash.ToLowerInvariant()
$item = Get-Item -LiteralPath $outputPath
Write-Output "built=$($item.FullName)"
Write-Output "bytes=$($item.Length)"
Write-Output "sha256=$hash"

$eventSource = Join-Path $portRoot 'tools\evsend.c'
$eventOutput = Join-Path $outputDirectory 'evsend'
& $Zig cc -target mipsel-linux-musleabi -mcpu=mips32r2 -Oz -static -s $eventSource -o $eventOutput
if ($LASTEXITCODE -ne 0) {
    throw "evsend cross-build failed with exit code $LASTEXITCODE"
}
$eventHash = (Get-FileHash -LiteralPath $eventOutput -Algorithm SHA256).Hash.ToLowerInvariant()
$eventItem = Get-Item -LiteralPath $eventOutput
Write-Output "event_sender=$($eventItem.FullName)"
Write-Output "event_sender_bytes=$($eventItem.Length)"
Write-Output "event_sender_sha256=$eventHash"
