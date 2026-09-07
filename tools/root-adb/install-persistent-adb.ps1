# C1-Slim / MP-D261 persistent root-ADB host installer.
# Original root-ADB method and script author: fwz233-RE (https://github.com/fwz233-RE)
# SPDX-License-Identifier: GPL-3.0-or-later

[CmdletBinding()]
param(
    [ValidateSet('Install', 'Verify', 'Uninstall')]
    [string]$Action = 'Install',
    [switch]$Reboot,
    [ValidateRange(30, 600)]
    [int]$ReconnectTimeoutSeconds = 300,
    [string]$AdbPath = ''
)

$ErrorActionPreference = 'Stop'
$expectedOriginalHash = 'c2b278b283e9bf851461d9e8f6edfd207cec3b120585f0e091777d163562e965'
$expectedInstalledHash = '626e4c5d600b543531337eb67220b0a7520d461211cc7ecafd36666d4f8905cb'
$deviceScriptPath = Join-Path $PSScriptRoot 'device-persistent-adb.sh'
$runRoot = Join-Path $PSScriptRoot "artifacts\persistent-adb-$([DateTimeOffset]::Now.ToString('yyyyMMdd-HHmmss'))"
$tempBase = [System.IO.Path]::GetFullPath([System.IO.Path]::GetTempPath())
$tempRoot = [System.IO.Path]::GetFullPath((Join-Path $tempBase "c1-persistent-adb-$([guid]::NewGuid().ToString('N'))"))
$remoteScript = '/dev/shm/c1-device-persistent-adb.sh'
$remoteCandidate = '/dev/shm/c1-S90usb.open-root-adb'
$script:Adb = $null
$script:Serial = $null
$script:RemoteFilesUploaded = $false

function Resolve-Adb {
    if ($AdbPath) {
        if (-not (Test-Path -LiteralPath $AdbPath -PathType Leaf)) {
            throw "ADB was not found at: $AdbPath"
        }
        return (Resolve-Path -LiteralPath $AdbPath).Path
    }

    $command = Get-Command adb -ErrorAction SilentlyContinue
    if ($null -eq $command) {
        $sdkAdb = Join-Path $env:LOCALAPPDATA 'Android\Sdk\platform-tools\adb.exe'
        if (Test-Path -LiteralPath $sdkAdb -PathType Leaf) {
            return (Resolve-Path -LiteralPath $sdkAdb).Path
        }
        throw 'ADB was not found. Add platform-tools to PATH or pass -AdbPath explicitly.'
    }
    return $command.Source
}

function Invoke-Adb {
    param(
        [Parameter(Mandatory)] [string[]]$Arguments,
        [switch]$AllowFailure,
        [switch]$WithoutSerial
    )

    [string[]]$allArguments = if ($WithoutSerial) {
        @($Arguments)
    } else {
        @('-s', $script:Serial) + @($Arguments)
    }
    $previousPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        $lines = & $script:Adb @allArguments 2>&1
        $exitCode = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $previousPreference
    }
    $output = ($lines | ForEach-Object { $_.ToString() }) -join [Environment]::NewLine
    if ($exitCode -ne 0 -and -not $AllowFailure) {
        throw "ADB failed ($exitCode): adb $($allArguments -join ' ')`n$output"
    }
    return [pscustomobject]@{ ExitCode = $exitCode; Output = $output }
}

function Invoke-Remote([string]$Command, [switch]$AllowFailure) {
    return Invoke-Adb -Arguments @('shell', $Command) -AllowFailure:$AllowFailure
}

function Invoke-CheckedRemote([string]$Command) {
    $marker = '__C1_REMOTE_EXIT='
    $wrapped = "$Command; c1_status=`$?; echo ${marker}`$c1_status"
    $result = Invoke-Remote $wrapped -AllowFailure
    $match = [regex]::Match($result.Output, "(?m)^$([regex]::Escape($marker))(\d+)\r?$")
    if (-not $match.Success) {
        throw "Remote command did not return an exit marker: $Command`n$($result.Output)"
    }
    $remoteExitCode = [int]$match.Groups[1].Value
    $cleanOutput = [regex]::Replace(
        $result.Output,
        "(?m)^$([regex]::Escape($marker))\d+\r?$",
        ''
    ).TrimEnd()
    if ($remoteExitCode -ne 0) {
        throw "Remote command failed ($remoteExitCode): $Command`n$cleanOutput"
    }
    return [pscustomobject]@{ ExitCode = 0; Output = $cleanOutput }
}

function Get-OnlyDevice {
    $devices = Invoke-Adb -Arguments @('devices') -WithoutSerial
    $deviceLines = @(
        $devices.Output -split "`r?`n" |
            Where-Object { $_ -match '^(\S+)\s+device$' }
    )
    if ($deviceLines.Count -ne 1) {
        throw "Exactly one connected ADB device is required; found $($deviceLines.Count)."
    }
    return [regex]::Match($deviceLines[0], '^(\S+)').Groups[1].Value
}

function Get-RemoteHash([string]$Path) {
    $result = Invoke-CheckedRemote "sha256sum $Path"
    return $result.Output.Split()[0].ToLowerInvariant()
}

function Assert-SafeBaseline {
    $identity = Invoke-Remote 'id'
    if ($identity.Output -notmatch 'uid=0\(root\)') {
        throw 'A root ADB shell is required. Complete the temporary root-ADB guide first.'
    }

    $compatible = (Invoke-CheckedRemote "tr '\000' '\n' < /proc/device-tree/compatible").Output
    if (@($compatible -split "`r?`n" | Where-Object { $_ -eq 'ingenic,halley6_v20' }).Count -ne 1) {
        throw "Unsupported device-tree compatible value: $compatible"
    }

    Invoke-CheckedRemote 'test -x /etc/init.d/usb/adb' | Out-Null
    Invoke-CheckedRemote 'test -f /etc/init.d/S90usb' | Out-Null

    $mounts = (Invoke-Remote 'mount').Output
    $rootMount = $mounts -split "`r?`n" |
        Where-Object { $_ -match '\son\s/\stype\s' } |
        Select-Object -First 1
    if (-not $rootMount -or $rootMount -notmatch '\(ro(?:,|\))') {
        throw "Root filesystem is not read-only: $rootMount"
    }
    $storageMount = $mounts -split "`r?`n" |
        Where-Object { $_ -match '\son\s/storage\stype\s' } |
        Select-Object -First 1
    if (-not $storageMount -or $storageMount -notmatch '\(rw(?:,|\))') {
        throw "Storage is not writable: $storageMount"
    }

    return [pscustomobject]@{
        Identity = $identity.Output
        Compatible = $compatible -replace "`r?`n", ','
        RootMount = $rootMount
        StorageMount = $storageMount
    }
}

function Build-Candidate([string]$OriginalPath, [string]$CandidatePath) {
    Invoke-Adb -Arguments @('pull', '/etc/init.d/S90usb', $OriginalPath) | Out-Null
    $actualHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $OriginalPath).Hash.ToLowerInvariant()
    if ($actualHash -ne $expectedOriginalHash) {
        throw "Pulled S90usb hash mismatch: expected $expectedOriginalHash, got $actualHash"
    }

    $originalText = [System.IO.File]::ReadAllText($OriginalPath)
    $disabledLine = "`t#/etc/init.d/usb/adb`t`$1"
    $enabledLine = "`t/etc/init.d/usb/adb`t`$1"
    $matches = ([regex]::Matches($originalText, [regex]::Escape($disabledLine))).Count
    if ($matches -ne 1) {
        throw "Expected exactly one disabled ADB startup line; found $matches."
    }
    $candidateText = $originalText.Replace($disabledLine, $enabledLine)
    [System.IO.File]::WriteAllText(
        $CandidatePath,
        $candidateText,
        [System.Text.UTF8Encoding]::new($false)
    )
    $candidateHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $CandidatePath).Hash.ToLowerInvariant()
    if ($candidateHash -ne $expectedInstalledHash) {
        throw "Generated candidate hash mismatch: expected $expectedInstalledHash, got $candidateHash"
    }
}

function Wait-ForRootReconnect([int]$TimeoutSeconds) {
    $deadline = [DateTimeOffset]::UtcNow.AddSeconds($TimeoutSeconds)
    do {
        Start-Sleep -Seconds 2
        $devices = Invoke-Adb -Arguments @('devices') -WithoutSerial -AllowFailure
        $connected = @(
            $devices.Output -split "`r?`n" |
                Where-Object { $_ -match "^$([regex]::Escape($script:Serial))\s+device$" }
        ).Count -eq 1
        if ($connected) {
            $probe = Invoke-Adb -Arguments @(
                '-s', $script:Serial, 'shell', 'id'
            ) -WithoutSerial -AllowFailure
            if ($probe.ExitCode -eq 0 -and $probe.Output -match 'uid=0\(root\)') {
                return
            }
        }
    } while ([DateTimeOffset]::UtcNow -lt $deadline)
    throw "Device did not reconnect as root ADB within $TimeoutSeconds seconds. See PERSISTENT.md recovery notes."
}

function Write-Evidence([string]$Name, [string]$Content) {
    $Content | Set-Content -Encoding utf8 -LiteralPath (Join-Path $runRoot $Name)
}

if (-not (Test-Path -LiteralPath $deviceScriptPath -PathType Leaf)) {
    throw "Device helper is missing: $deviceScriptPath"
}
if (-not $tempRoot.StartsWith($tempBase, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Unsafe temporary path: $tempRoot"
}

New-Item -ItemType Directory -Path $runRoot, $tempRoot | Out-Null
$localOriginal = Join-Path $tempRoot 'S90usb.original'
$localCandidate = Join-Path $tempRoot 'S90usb.open-root-adb'
$uploadScript = Join-Path $tempRoot 'device-persistent-adb.sh'
$deviceScriptText = [System.IO.File]::ReadAllText($deviceScriptPath).Replace("`r`n", "`n")
[System.IO.File]::WriteAllText(
    $uploadScript,
    $deviceScriptText,
    [System.Text.UTF8Encoding]::new($false)
)

$script:Adb = Resolve-Adb
try {
    $script:Serial = Get-OnlyDevice
    $baseline = Assert-SafeBaseline
    $currentHash = Get-RemoteHash '/etc/init.d/S90usb'
    if ($currentHash -notin @($expectedOriginalHash, $expectedInstalledHash)) {
        throw "Device S90usb has an unsupported hash: $currentHash"
    }

    if ($Action -eq 'Install' -and $currentHash -eq $expectedOriginalHash) {
        Build-Candidate $localOriginal $localCandidate
    }

    Write-Evidence 'baseline.txt' (@(
        "action=$Action"
        "serial=$($script:Serial)"
        "identity=$($baseline.Identity)"
        "compatible=$($baseline.Compatible)"
        "root_mount=$($baseline.RootMount)"
        "storage_mount=$($baseline.StorageMount)"
        "original_sha256=$expectedOriginalHash"
        "installed_sha256=$expectedInstalledHash"
        "device_before_sha256=$currentHash"
        'authentication=none'
    ) -join [Environment]::NewLine)

    Invoke-Adb -Arguments @('push', $uploadScript, $remoteScript) | Out-Null
    Invoke-Remote "chmod 700 $remoteScript" | Out-Null
    $script:RemoteFilesUploaded = $true

    $localScriptHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $uploadScript).Hash.ToLowerInvariant()
    $remoteScriptHash = Get-RemoteHash $remoteScript
    if ($remoteScriptHash -ne $localScriptHash) {
        throw 'Uploaded device helper hash mismatch.'
    }
    Invoke-CheckedRemote "sh -n $remoteScript" | Out-Null

    if (Test-Path -LiteralPath $localCandidate -PathType Leaf) {
        Invoke-Adb -Arguments @('push', $localCandidate, $remoteCandidate) | Out-Null
        Invoke-Remote "chmod 600 $remoteCandidate" | Out-Null
        if ((Get-RemoteHash $remoteCandidate) -ne $expectedInstalledHash) {
            throw 'Uploaded S90usb candidate hash mismatch.'
        }
    }

    $actionName = $Action.ToLowerInvariant()
    $operation = Invoke-CheckedRemote "$remoteScript $actionName $expectedOriginalHash $expectedInstalledHash"
    Write-Evidence 'operation.txt' $operation.Output

    $postOperation = Assert-SafeBaseline
    if ($Action -in @('Install', 'Verify')) {
        $verification = Invoke-CheckedRemote "$remoteScript verify $expectedOriginalHash $expectedInstalledHash"
        Write-Evidence 'verification-before-reboot.txt' $verification.Output
    }

    if ($Reboot) {
        if ($Action -eq 'Uninstall') {
            Invoke-Adb -Arguments @('reboot') -AllowFailure | Out-Null
            Write-Warning 'The original MTP-only startup was restored. ADB is expected to disappear after this reboot, so final MTP-only verification is manual.'
        } else {
            Invoke-Adb -Arguments @('reboot') -AllowFailure | Out-Null
            Wait-ForRootReconnect $ReconnectTimeoutSeconds
            $postBoot = Assert-SafeBaseline
            $persistentVerifier = '/storage/c1/recovery/open-adb/device-open-adb.sh'
            $postBootVerification = Invoke-CheckedRemote "sh $persistentVerifier verify $expectedOriginalHash $expectedInstalledHash"
            Write-Evidence 'verification-after-reboot.txt' (@(
                $postBootVerification.Output
                "identity=$($postBoot.Identity)"
                "root_mount=$($postBoot.RootMount)"
            ) -join [Environment]::NewLine)
        }
    }
} finally {
    if ($script:RemoteFilesUploaded -and $null -ne $script:Serial) {
        Invoke-Remote "rm -f $remoteScript $remoteCandidate" -AllowFailure | Out-Null
    }
    if (Test-Path -LiteralPath $tempRoot) {
        $resolvedTempRoot = [System.IO.Path]::GetFullPath($tempRoot)
        if ($resolvedTempRoot.StartsWith($tempBase, [System.StringComparison]::OrdinalIgnoreCase)) {
            Remove-Item -LiteralPath $resolvedTempRoot -Recurse -Force -ErrorAction SilentlyContinue
        }
    }
}

Write-Host "$Action completed."
Write-Host "Original S90usb SHA-256: $expectedOriginalHash"
Write-Host "Installed S90usb SHA-256: $expectedInstalledHash"
Write-Host "Evidence: $runRoot"
if ($Action -eq 'Install' -and -not $Reboot) {
    Write-Warning 'The file change is installed but cold-start ADB has not been verified. Re-run with -Action Verify -Reboot.'
}
