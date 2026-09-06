$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$tracked=@(& git -C $root ls-files)
if($LASTEXITCODE -ne 0){throw 'Unable to enumerate tracked files'}
$trackedFiles=@($tracked | ForEach-Object {Get-Item -LiteralPath (Join-Path $root $_)})
$forbidden=@(
    'mpenMain', '*.lav', '*.dat', '*.log', '*.zip', '*.tar.gz',
    'id_rsa', 'id_ed25519', '*.pem', '*.key'
)
$bad=@()
foreach($pattern in $forbidden){
    $bad += $trackedFiles | Where-Object {$_.Name -like $pattern}
}
$large=$trackedFiles | Where-Object {$_.Length -gt 50MB}
$secretHits=& git -C $root grep -n -I -E '(github_pat_|ghp_|xox[baprs]-|BEGIN [A-Z ]*PRIVATE KEY|api[_-]?key[[:space:]]*[:=]|password[[:space:]]*[:=])' -- . ':(exclude)tools/preflight.ps1'
if($bad){$bad|Select-Object FullName,Length|Format-Table;throw 'Forbidden private/content files found'}
if($large){$large|Select-Object FullName,Length|Format-Table;throw 'Files over 50 MiB found'}
if($LASTEXITCODE -eq 0){$secretHits;throw 'Possible secret material found'}
if($LASTEXITCODE -gt 1){throw 'Secret scan failed'}
Write-Output 'PASS: no forbidden private content, oversized files, or obvious credentials.'
exit 0
