[CmdletBinding()]
param(
    [string]$Port = '',
    [ValidateSet('auto', 'rev1_3', 'rev3_x')]
    [string]$Profile = 'auto',
    [switch]$ListOnly,
    [switch]$SelfTest
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$Repo = 'waveshareteam/ESP32-P4-Platform'
$Workflow = 'product-firmware.yml'
$ArtifactPrefix = 'esp32-p4-platform-firmware-'
$FlashSizeBytes = [int64](32 * 1024 * 1024)
$Profiles = @('rev1_3', 'rev3_x')

function Test-HeadSha([string]$Value) { return $Value -match '^[0-9a-fA-F]{40}$' }
function Test-Port([string]$Value) { return $Value -match '^COM[1-9][0-9]*$' }
function Test-C6Name([string]$Value) { return $Value -match '(?i)esp32c6|(?:^|[^a-z0-9])c6(?:[^a-z0-9]|$)' }

function Get-StringSha256([string]$Value) {
    $bytes = [System.Text.Encoding]::UTF8.GetBytes($Value)
    $algorithm = [System.Security.Cryptography.SHA256]::Create()
    try { return [System.BitConverter]::ToString($algorithm.ComputeHash($bytes)).Replace('-', '').ToLowerInvariant() }
    finally { $algorithm.Dispose() }
}

function Get-FileSha256([string]$Path) {
    $stream = $null; $algorithm = $null
    try {
        $stream = [System.IO.File]::OpenRead($Path)
        $algorithm = [System.Security.Cryptography.SHA256]::Create()
        return [System.BitConverter]::ToString($algorithm.ComputeHash($stream)).Replace('-', '').ToLowerInvariant()
    }
    finally {
        if ($null -ne $stream) { $stream.Dispose() }
        if ($null -ne $algorithm) { $algorithm.Dispose() }
    }
}

function Get-StreamSha256([System.IO.Stream]$Stream) {
    $algorithm = $null
    try {
        if (-not $Stream.CanSeek) { throw 'A seekable stream is required for checksum verification.' }
        $Stream.Position = 0
        $algorithm = [System.Security.Cryptography.SHA256]::Create()
        return [System.BitConverter]::ToString($algorithm.ComputeHash($Stream)).Replace('-', '').ToLowerInvariant()
    }
    finally {
        if ($null -ne $Stream -and $Stream.CanSeek) { $Stream.Position = 0 }
        if ($null -ne $algorithm) { $algorithm.Dispose() }
    }
}

function New-SafeTemporaryDirectory {
    $tempRoot = [System.IO.Path]::GetFullPath([System.IO.Path]::GetTempPath()).TrimEnd([System.IO.Path]::DirectorySeparatorChar, [System.IO.Path]::AltDirectorySeparatorChar)
    for ($index = 0; $index -lt 20; $index++) {
        $candidate = Join-Path $tempRoot ('p4-product-firmware-' + [System.IO.Path]::GetRandomFileName())
        if (-not (Test-Path -LiteralPath $candidate)) { New-Item -ItemType Directory -Path $candidate | Out-Null; return $candidate }
    }
    throw 'Unable to create a temporary directory.'
}

function Remove-SafeTemporaryDirectory([string]$Path) {
    $tempRoot = [System.IO.Path]::GetFullPath([System.IO.Path]::GetTempPath()).TrimEnd([System.IO.Path]::DirectorySeparatorChar, [System.IO.Path]::AltDirectorySeparatorChar) + [System.IO.Path]::DirectorySeparatorChar
    $candidate = [System.IO.Path]::GetFullPath($Path)
    if (-not $candidate.StartsWith($tempRoot, [System.StringComparison]::OrdinalIgnoreCase) -or -not ([System.IO.Path]::GetFileName($candidate).StartsWith('p4-product-firmware-', [System.StringComparison]::OrdinalIgnoreCase))) { throw 'Refusing to remove a directory outside the dedicated temporary location.' }
    if (Test-Path -LiteralPath $candidate) { Remove-Item -LiteralPath $candidate -Recurse -Force }
}

function Test-RelativePackagePath([string]$PackageRoot, [string]$RelativePath) {
    if ([string]::IsNullOrWhiteSpace($RelativePath) -or $RelativePath -match '[\\:]' -or [System.IO.Path]::IsPathRooted($RelativePath)) { return $false }
    $parts = $RelativePath.Split('/')
    if ($parts.Count -eq 0 -or @($parts | Where-Object { [string]::IsNullOrWhiteSpace($_) -or $_ -eq '.' -or $_ -eq '..' }).Count -gt 0) { return $false }
    $root = [System.IO.Path]::GetFullPath($PackageRoot).TrimEnd([System.IO.Path]::DirectorySeparatorChar, [System.IO.Path]::AltDirectorySeparatorChar) + [System.IO.Path]::DirectorySeparatorChar
    $candidate = [System.IO.Path]::GetFullPath((Join-Path $PackageRoot ($parts -join [System.IO.Path]::DirectorySeparatorChar)))
    return $candidate.StartsWith($root, [System.StringComparison]::OrdinalIgnoreCase)
}

function Get-SafePackageFile([string]$PackageDir, [string]$RelativePath) {
    if (-not (Test-RelativePackagePath $PackageDir $RelativePath)) { throw "Unsafe package path: $RelativePath" }
    if (Test-C6Name $RelativePath) { throw "C6-named package path is not permitted: $RelativePath" }
    $fullPath = Join-Path $PackageDir (($RelativePath.Split('/') -join [System.IO.Path]::DirectorySeparatorChar))
    if (-not (Test-Path -LiteralPath $fullPath -PathType Leaf)) { throw "Package file is missing: $RelativePath" }
    $item = Get-Item -LiteralPath $fullPath -Force
    if (($item.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0 -or ($item.Attributes -band [System.IO.FileAttributes]::Directory) -ne 0 -or $item.Length -le 0) { throw "Package file must be a non-empty regular file: $RelativePath" }
    return [pscustomobject]@{ Path = $item.FullName; Size = [int64]$item.Length }
}

function Get-ChecksumTable([string]$PackageDir) {
    $checksumPath = Join-Path $PackageDir 'SHA256SUMS'
    if (-not (Test-Path -LiteralPath $checksumPath -PathType Leaf)) { throw 'SHA256SUMS is missing.' }
    $table = @{}
    foreach ($line in @(Get-Content -LiteralPath $checksumPath)) {
        if ([string]::IsNullOrWhiteSpace($line)) { continue }
        $match = [regex]::Match($line, '^([0-9a-fA-F]{64}) \*([^\\/]+(?:/[^\\/]+)*)$')
        if (-not $match.Success -or $table.ContainsKey($match.Groups[2].Value) -or (Test-C6Name $match.Groups[2].Value)) { throw 'SHA256SUMS contains an invalid, duplicate, or C6 entry.' }
        $table[$match.Groups[2].Value] = $match.Groups[1].Value.ToLowerInvariant()
    }
    if ($table.Count -eq 0) { throw 'SHA256SUMS has no entries.' }
    return $table
}

function Assert-VerifiedPackageFile([string]$PackageDir, [string]$RelativePath, [int64]$ExpectedSize, [string]$ExpectedSha256, $Checksums) {
    if (-not $Checksums.ContainsKey($RelativePath) -or $ExpectedSha256 -notmatch '^[0-9a-fA-F]{64}$') { throw "Unsafe or unchecked package file: $RelativePath" }
    $file = Get-SafePackageFile $PackageDir $RelativePath
    if ($file.Size -ne $ExpectedSize -or -not [string]::Equals($Checksums[$RelativePath], $ExpectedSha256, [System.StringComparison]::OrdinalIgnoreCase) -or -not [string]::Equals((Get-FileSha256 $file.Path), $ExpectedSha256, [System.StringComparison]::OrdinalIgnoreCase)) { throw "Package checksum or size verification failed: $RelativePath" }
    return [pscustomobject]@{ Path = $file.Path; Size = $file.Size; Sha256 = $ExpectedSha256.ToLowerInvariant() }
}

function Convert-Offset([string]$Value) {
    if ($Value -notmatch '^0x[0-9a-fA-F]+$') { throw "Invalid flash offset: $Value" }
    return [Convert]::ToInt64($Value.Substring(2), 16)
}

function Get-VerifiedImageDigest([string]$ManifestPath, $Plan, $Merged) {
    $parts = @('manifest=' + (Get-FileSha256 $ManifestPath))
    foreach ($entry in @($Plan | Sort-Object Offset)) { $parts += ('{0}:{1}:{2}' -f $entry.OffsetText, $entry.PathInPackage, $entry.Sha256) }
    $parts += ('merged.bin:' + $Merged.Sha256)
    return Get-StringSha256 (($parts -join "`n") + "`n")
}

function Assert-RevisionContract($Manifest, [string]$ProfileName) {
    if ($null -eq $Manifest.revision_contract -or $null -eq $Manifest.config_evidence) { throw 'Manifest revision contract or config evidence is missing.' }
    $contract = $Manifest.revision_contract; $evidence = $Manifest.config_evidence
    $expectedLess = if ($ProfileName -eq 'rev1_3') { 'y' } else { 'n_or_unset' }
    $expectedMinimum = if ($ProfileName -eq 'rev1_3') { '100' } else { '300' }
    if ([string]$contract.profile -ne $ProfileName -or [string]$contract.selects_rev_less_v3 -ne $expectedLess -or [string]$contract.revision_minimum -ne $expectedMinimum -or -not ($contract.opposite_minimum_selected -is [bool]) -or [bool]$contract.opposite_minimum_selected) { throw 'Manifest revision_contract is inconsistent with the selected profile.' }
    if ([string]$evidence.CONFIG_ESPTOOLPY_FLASHSIZE_32MB -ne 'y') { throw 'Manifest config evidence does not prove 32 MiB flash.' }
    if ($ProfileName -eq 'rev1_3') {
        if ([string]$evidence.CONFIG_ESP32P4_SELECTS_REV_LESS_V3 -ne 'y' -or [string]$evidence.CONFIG_ESP32P4_REV_MIN_100 -ne 'y' -or [string]$evidence.CONFIG_ESP32P4_REV_MIN_300 -eq 'y') { throw 'Manifest config evidence is inconsistent with rev1_3.' }
    } elseif ([string]$evidence.CONFIG_ESP32P4_SELECTS_REV_LESS_V3 -eq 'y' -or [string]$evidence.CONFIG_ESP32P4_REV_MIN_100 -eq 'y' -or [string]$evidence.CONFIG_ESP32P4_REV_MIN_300 -ne 'y') { throw 'Manifest config evidence is inconsistent with rev3_x.' }
}

function Assert-NoEraseCommand([string]$PackageDir) {
    foreach ($relative in @('flash_args', 'flash.sh', 'flash.bat')) {
        $file = Get-SafePackageFile $PackageDir $relative
        $content = [System.IO.File]::ReadAllText($file.Path)
        if ($content -match '(?i)erase') { throw "Erase command/string is not permitted: $relative" }
        if ($content -notmatch '(?i)\bwrite_flash\b') { throw "Split flash helper lacks write_flash: $relative" }
    }
}

function Assert-PackageHasNoC6Names([string]$PackageDir) {
    $root = [System.IO.Path]::GetFullPath($PackageDir).TrimEnd([System.IO.Path]::DirectorySeparatorChar, [System.IO.Path]::AltDirectorySeparatorChar) + [System.IO.Path]::DirectorySeparatorChar
    foreach ($item in @(Get-ChildItem -LiteralPath $PackageDir -Recurse -Force)) {
        $fullName = [System.IO.Path]::GetFullPath($item.FullName)
        if (-not $fullName.StartsWith($root, [System.StringComparison]::OrdinalIgnoreCase)) { throw 'Package entry escapes the downloaded artifact.' }
        $relative = $fullName.Substring($root.Length).Replace([System.IO.Path]::DirectorySeparatorChar, '/')
        if (Test-C6Name $relative) { throw "C6-named package entry is not permitted: $relative" }
    }
}

function Get-FlashSettings([string]$PackageDir, $Plan) {
    $path = Join-Path $PackageDir 'flasher_args.json'
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw 'flasher_args.json is missing.' }
    $flasher = Get-Content -LiteralPath $path -Raw | ConvertFrom-Json
    if ([string]$flasher.target -ne 'esp32p4' -or $null -eq $flasher.flash_settings -or $null -eq $flasher.flash_files) { throw 'flasher_args.json is incomplete or targets the wrong chip.' }
    $settings = $flasher.flash_settings
    foreach ($name in @('flash_mode', 'flash_freq', 'flash_size')) {
        if ([string]::IsNullOrWhiteSpace([string]$settings.$name) -or [string]$settings.$name -match '\s|[;|&]') { throw "Unsafe flash setting: $name" }
    }
    if ([string]$settings.flash_size -ine '32MB') { throw 'flasher_args.json must select 32MB flash.' }
    foreach ($entry in $Plan) {
        $value = $flasher.flash_files.($entry.OffsetText)
        if ([string]$value -ne $entry.PathInPackage) { throw 'flasher_args.json does not match the verified split flash plan.' }
    }
    return $settings
}

function Test-PackageManifest([string]$PackageDir, [string]$FinalSha, [string]$SelectedRunId, [string]$SelectedRunAttempt, [string]$ProfileName) {
    Assert-PackageHasNoC6Names $PackageDir
    $manifestPath = Join-Path $PackageDir 'manifest.json'
    if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) { throw 'manifest.json is missing.' }
    $manifestText = Get-Content -LiteralPath $manifestPath -Raw
    $manifest = $manifestText | ConvertFrom-Json
    if ([string]$manifest.schema_version -ne '2' -or [string]$manifest.target -ne 'esp32p4' -or [string]$manifest.profile -ne $ProfileName) { throw 'Manifest schema, target, or profile is invalid.' }
    if (-not (Test-HeadSha ([string]$manifest.source_sha)) -or -not [string]::Equals([string]$manifest.source_sha, $FinalSha, [System.StringComparison]::OrdinalIgnoreCase) -or -not (Test-HeadSha ([string]$manifest.pr_head_sha)) -or -not [string]::Equals([string]$manifest.pr_head_sha, $FinalSha, [System.StringComparison]::OrdinalIgnoreCase)) { throw 'Manifest source or PR SHA does not match local HEAD.' }
    if ([string]$manifest.event -ne 'pull_request' -or [string]$manifest.run_id -ne $SelectedRunId -or [string]$manifest.run_attempt -ne $SelectedRunAttempt -or [string]$manifest.idf_version -ne 'v5.5.5') { throw 'Manifest event, run identity, or IDF version is invalid.' }
    if ([string]$manifest.capacity -ne '32MiB' -or [int64]$manifest.capacity_bytes -ne $FlashSizeBytes -or -not ($manifest.c6_binary_included -is [bool]) -or [bool]$manifest.c6_binary_included -or -not ($manifest.erase_required -is [bool]) -or [bool]$manifest.erase_required -or [string]$manifest.flash_by_default -ne 'split_files' -or [int]$manifest.retention_days -ne 14) { throw 'Manifest capacity, C6, erase, default-flash, or retention contract is invalid.' }
    Assert-RevisionContract $manifest $ProfileName
    $checksums = Get-ChecksumTable $PackageDir
    $flashFiles = @($manifest.flash_files)
    if ($flashFiles.Count -lt 1) { throw 'Manifest must contain at least one split flash file.' }
    $plan = @(); $offsets = @{}; $paths = @{}
    foreach ($entry in $flashFiles) {
        $offsetText = [string]$entry.offset; $pathInPackage = [string]$entry.path
        if (Test-C6Name $pathInPackage) { throw "C6-named flash file is not permitted: $pathInPackage" }
        $offset = Convert-Offset $offsetText
        if ($offsets.ContainsKey($offset) -or $paths.ContainsKey($pathInPackage) -or [int64]$entry.size -le 0) { throw 'Manifest split flash files have a duplicate offset/path or invalid size.' }
        $offsets[$offset] = $true; $paths[$pathInPackage] = $true
        $file = Assert-VerifiedPackageFile $PackageDir $pathInPackage ([int64]$entry.size) ([string]$entry.sha256) $checksums
        if ($offset + $file.Size -gt $FlashSizeBytes) { throw 'Split flash file is outside the 32 MiB capacity.' }
        $plan += [pscustomobject]@{ Offset = $offset; OffsetText = ('0x{0:x}' -f $offset); PathInPackage = $pathInPackage; Path = $file.Path; Size = $file.Size; Sha256 = $file.Sha256 }
    }
    $orderedPlan = @($plan | Sort-Object Offset)
    for ($index = 1; $index -lt $orderedPlan.Count; $index++) { if ($orderedPlan[$index - 1].Offset + $orderedPlan[$index - 1].Size -gt $orderedPlan[$index].Offset) { throw 'Split flash plan contains overlapping ranges.' } }
    if ($null -eq $manifest.merged_image -or [string]$manifest.merged_image.path -ne 'merged.bin' -or [int64]$manifest.merged_image.size -ne $FlashSizeBytes -or [string]$manifest.merged_image.flash_by_default -ne 'False') { throw 'Manifest merged image must be exactly 32 MiB and not the default flash method.' }
    $merged = Assert-VerifiedPackageFile $PackageDir 'merged.bin' ([int64]$manifest.merged_image.size) ([string]$manifest.merged_image.sha256) $checksums
    if ($checksums.Count -ne ($plan.Count + 1)) { throw 'SHA256SUMS must cover exactly every split file and merged.bin.' }
    foreach ($pathInPackage in @($paths.Keys + 'merged.bin')) { if (-not $checksums.ContainsKey($pathInPackage)) { throw 'SHA256SUMS is incomplete.' } }
    Assert-NoEraseCommand $PackageDir
    $settings = Get-FlashSettings $PackageDir $orderedPlan
    return [pscustomobject]@{ Plan = $orderedPlan; Settings = $settings; ImageDigest = (Get-VerifiedImageDigest $manifestPath $orderedPlan $merged) }
}

function Lock-VerifiedFlashPlan($Plan) {
    $locks = @()
    $stream = $null
    try {
        foreach ($entry in $Plan) {
            $stream = [System.IO.File]::Open($entry.Path, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Read, [System.IO.FileShare]::Read)
            if ([int64]$stream.Length -ne [int64]$entry.Size -or -not [string]::Equals((Get-StreamSha256 $stream), [string]$entry.Sha256, [System.StringComparison]::OrdinalIgnoreCase)) { throw "Locked package checksum or size verification failed: $($entry.PathInPackage)" }
            $locks += [pscustomobject]@{ Entry = $entry; Stream = $stream }
            $stream = $null
        }
        return @($locks)
    }
    catch {
        if ($null -ne $stream) { $stream.Dispose() }
        foreach ($lock in $locks) { $lock.Stream.Dispose() }
        throw
    }
}

function Close-VerifiedFlashPlanLocks($Locks) {
    foreach ($lock in @($Locks)) { if ($null -ne $lock -and $null -ne $lock.Stream) { $lock.Stream.Dispose() } }
}

function Find-PackageDirectory([string]$DownloadDir) {
    $manifests = @(Get-ChildItem -LiteralPath $DownloadDir -Recurse -File -Filter 'manifest.json')
    if ($manifests.Count -ne 1) { throw 'Expected exactly one manifest.json in the downloaded artifact.' }
    return $manifests[0].DirectoryName
}

function Resolve-Executable([string]$Name) {
    $command = Get-Command $Name -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($null -eq $command -or [string]::IsNullOrWhiteSpace($command.Source)) { throw "$Name was not found on PATH." }
    return $command.Source
}

function Assert-Repository([string]$GitExe) {
    $repoRoot = (& $GitExe -C (Join-Path $PSScriptRoot '..') rev-parse --show-toplevel 2>&1 | Out-String).Trim()
    if ($LASTEXITCODE -ne 0) { throw 'Unable to resolve the repository root.' }
    $expectedRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..')).TrimEnd([System.IO.Path]::DirectorySeparatorChar, [System.IO.Path]::AltDirectorySeparatorChar)
    if (-not [string]::Equals([System.IO.Path]::GetFullPath($repoRoot).TrimEnd([System.IO.Path]::DirectorySeparatorChar, [System.IO.Path]::AltDirectorySeparatorChar), $expectedRoot, [System.StringComparison]::OrdinalIgnoreCase)) { throw 'This tool must run from its own repository root.' }
}

function Resolve-FinalSha([string]$GitExe) {
    $sha = (& $GitExe -C (Join-Path $PSScriptRoot '..') rev-parse HEAD 2>&1 | Out-String).Trim()
    if ($LASTEXITCODE -ne 0 -or -not (Test-HeadSha $sha)) { throw 'Unable to resolve a full local HEAD SHA.' }
    return $sha.ToLowerInvariant()
}

function Assert-TrackedWorktreeClean([string]$GitExe) {
    $status = (& $GitExe -C (Join-Path $PSScriptRoot '..') status --porcelain=v1 --untracked-files=no 2>&1 | Out-String)
    if ($LASTEXITCODE -ne 0) { throw 'Unable to determine tracked working-tree state.' }
    if (-not [string]::IsNullOrWhiteSpace($status)) { throw 'Refusing to continue: tracked files have staged or unstaged changes.' }
}

function Resolve-CurrentBranch([string]$GitExe) {
    $branch = (& $GitExe -C (Join-Path $PSScriptRoot '..') symbolic-ref --quiet --short HEAD 2>&1 | Out-String).Trim()
    if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($branch)) { throw 'Refusing to continue: check out a non-detached branch first.' }
    return $branch
}

function Assert-ReadyPullRequest([string]$GhExe, [string]$Branch, [string]$FinalSha) {
    $raw = (& $GhExe pr list --repo $Repo --head $Branch --state open --limit 2 --json number,state,isDraft,headRefName,headRefOid 2>&1 | Out-String)
    if ($LASTEXITCODE -ne 0) { throw 'Unable to query the pull request for the current branch.' }
    $pullRequests = @($raw | ConvertFrom-Json)
    if ($pullRequests.Count -ne 1) { throw 'Refusing to continue: the current branch must have exactly one open pull request.' }
    $pullRequest = $pullRequests[0]
    if ([string]$pullRequest.state -ine 'OPEN' -or [bool]$pullRequest.isDraft -or [string]$pullRequest.headRefName -ne $Branch -or -not (Test-HeadSha ([string]$pullRequest.headRefOid)) -or -not [string]::Equals([string]$pullRequest.headRefOid, $FinalSha, [System.StringComparison]::OrdinalIgnoreCase)) { throw 'Refusing to continue: the pull request must be open, non-draft, match the current branch, and have exact local HEAD.' }
    return $pullRequest
}

function Resolve-SuccessfulRun([string]$GhExe, [string]$FinalSha, [string]$Branch) {
    $raw = (& $GhExe run list --repo $Repo --workflow $Workflow --commit $FinalSha --status success --limit 20 --json databaseId,headSha,headBranch,event,attempt,createdAt 2>&1 | Out-String)
    if ($LASTEXITCODE -ne 0) { throw 'Unable to list successful product-firmware workflow runs.' }
    $runs = @($raw | ConvertFrom-Json | Where-Object { [string]$_.event -eq 'pull_request' -and [string]$_.headBranch -eq $Branch -and [string]::Equals([string]$_.headSha, $FinalSha, [System.StringComparison]::OrdinalIgnoreCase) -and [string]$_.databaseId -match '^[1-9][0-9]*$' -and [string]$_.attempt -match '^[1-9][0-9]*$' } | Sort-Object createdAt -Descending)
    if ($runs.Count -lt 1) { throw 'No successful pull_request product-firmware workflow run exists for the current branch and local HEAD.' }
    return $runs[0]
}

function Get-LaunchContext([string]$GitExe, [string]$GhExe, [string]$ProfileName) {
    Assert-Repository $GitExe
    $finalSha = Resolve-FinalSha $GitExe
    Assert-TrackedWorktreeClean $GitExe
    $branch = Resolve-CurrentBranch $GitExe
    $pullRequest = Assert-ReadyPullRequest $GhExe $branch $finalSha
    $run = Resolve-SuccessfulRun $GhExe $finalSha $branch
    return [pscustomobject]@{ FinalSha = $finalSha; Branch = $branch; PullRequestNumber = [string]$pullRequest.number; PullRequestHead = [string]$pullRequest.headRefOid; RunId = [string]$run.databaseId; RunAttempt = [string]$run.attempt; Profile = $ProfileName; Artifact = ($ArtifactPrefix + $ProfileName + '-' + $finalSha) }
}

function Assert-LaunchContextUnchanged($LockedContext, [string]$GitExe, [string]$GhExe, [string]$ProfileName) {
    $current = Get-LaunchContext $GitExe $GhExe $ProfileName
    foreach ($name in @('FinalSha', 'Branch', 'PullRequestNumber', 'PullRequestHead', 'RunId', 'RunAttempt', 'Profile', 'Artifact')) { if (-not [string]::Equals([string]$LockedContext.$name, [string]$current.$name, [System.StringComparison]::OrdinalIgnoreCase)) { throw 'Repository, pull-request, workflow, or profile context changed after the window opened. Close and restart this tool.' } }
    return $current
}

function Get-PresentP4PortRecords {
    if ($null -eq (Get-Command Get-PnpDevice -ErrorAction SilentlyContinue)) { throw 'Get-PnpDevice is unavailable; do not flash until present-device validation is available.' }
    $records = @()
    foreach ($device in @(Get-PnpDevice -PresentOnly | Where-Object { [string]$_.InstanceId -match '(?i)VID_303A' })) {
        $name = [string]$device.FriendlyName; if ([string]::IsNullOrWhiteSpace($name)) { $name = [string]$device.Name }
        $match = [regex]::Match($name, '\\((COM[1-9][0-9]*)\\)')
        if ($match.Success) { $records += [pscustomobject]@{ Port = $match.Groups[1].Value.ToUpperInvariant(); IdentityHash = (Get-StringSha256 ([string]$device.InstanceId)) } }
    }
    return $records
}

function Resolve-PresentP4Port([string]$RequestedPort) {
    $records = @(Get-PresentP4PortRecords)
    if ([string]::IsNullOrWhiteSpace($RequestedPort)) { if ($records.Count -ne 1) { throw 'Unable to identify exactly one present VID_303A USB serial port; pass -Port COMx.' }; return $records[0] }
    $requested = $RequestedPort.Trim().ToUpperInvariant()
    if (-not (Test-Port $requested)) { throw 'Port must be COM followed by a positive number, for example COM12.' }
    $matches = @($records | Where-Object { $_.Port -eq $requested })
    if ($matches.Count -ne 1) { throw 'The selected port must map to exactly one present VID_303A USB serial instance.' }
    return $matches[0]
}

function Get-P4FlashIdentity([string]$Output) {
    $chip = [regex]::Match($Output, '(?im)(?:Chip is|Chip type:)\s*ESP32-P4\s*\(revision\s+v?([0-9]+)\.([0-9]+)\)')
    $flash = [regex]::Match($Output, '(?im)(?:Detected flash size|Flash size)\s*:\s*([0-9]+)\s*MB\b')
    if (-not $chip.Success -or -not $flash.Success) { throw 'flash_id output did not provide a parseable ESP32-P4 revision and flash size.' }
    $major = [int]$chip.Groups[1].Value; $minor = [int]$chip.Groups[2].Value; $flashBytes = [int64]([int]$flash.Groups[1].Value) * 1024 * 1024
    if ($major -lt 1 -or $flashBytes -lt $FlashSizeBytes) { throw 'Connected target must be ESP32-P4 revision 1.0 or later with at least 32 MiB flash.' }
    return [pscustomobject]@{ Revision = ('{0}.{1}' -f $major, $minor); Major = $major; FlashBytes = $flashBytes }
}

function Resolve-ProfileForIdentity($Identity, [string]$RequestedProfile) {
    $detected = if ($Identity.Major -lt 3) { 'rev1_3' } else { 'rev3_x' }
    if ($RequestedProfile -ne 'auto' -and $RequestedProfile -ne $detected) { throw "Requested profile $RequestedProfile does not match detected ESP32-P4 revision v$($Identity.Revision), which requires $detected." }
    return $detected
}

function Probe-Target([string]$PythonExe, $PortContext, [string]$RequestedProfile) {
    $flashIdOutput = (& $PythonExe -m esptool --port $PortContext.Port --chip esp32p4 flash_id 2>&1 | Out-String)
    if ($LASTEXITCODE -ne 0) { throw 'flash_id failed.' }
    $identity = Get-P4FlashIdentity $flashIdOutput
    return [pscustomobject]@{ Identity = $identity; Profile = (Resolve-ProfileForIdentity $identity $RequestedProfile); FlashIdOutput = $flashIdOutput }
}

function Assert-TargetUnchanged($LockedPortContext, $LockedProbe, $CurrentPortContext, $CurrentProbe) {
    foreach ($item in @(
        @{ Name = 'port'; Locked = $LockedPortContext.Port; Current = $CurrentPortContext.Port },
        @{ Name = 'PnP identity'; Locked = $LockedPortContext.IdentityHash; Current = $CurrentPortContext.IdentityHash },
        @{ Name = 'chip revision'; Locked = $LockedProbe.Identity.Revision; Current = $CurrentProbe.Identity.Revision },
        @{ Name = 'flash size'; Locked = $LockedProbe.Identity.FlashBytes; Current = $CurrentProbe.Identity.FlashBytes },
        @{ Name = 'profile'; Locked = $LockedProbe.Profile; Current = $CurrentProbe.Profile }
    )) {
        if (-not [string]::Equals([string]$item.Locked, [string]$item.Current, [System.StringComparison]::OrdinalIgnoreCase)) { throw "Target $($item.Name) changed after the initial probe. Close and restart this tool." }
    }
}

function Get-StatePath { return Join-Path $env:LOCALAPPDATA 'Waveshare\CI-Firmware\ESP32-P4-Platform\hardware-state.json' }
function Read-HistoricalState([string]$FinalSha) { $path = Get-StatePath; if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { return $null }; $state = Get-Content -LiteralPath $path -Raw | ConvertFrom-Json; if ([string]$state.final_sha -ne $FinalSha -or -not [bool]$state.hardware_pass) { return $null }; return $state }
function Save-HardwarePass($VerifiedContext) {
    $path = Get-StatePath; $directory = Split-Path -Parent $path
    if (-not (Test-Path -LiteralPath $directory)) { New-Item -ItemType Directory -Path $directory | Out-Null }
    [pscustomobject]@{ final_sha = $VerifiedContext.FinalSha; hardware_pass = $true; port = $VerifiedContext.Port; pnp_identity_hash = $VerifiedContext.PnpIdentityHash; workflow = $Workflow; run_id = $VerifiedContext.RunId; run_attempt = $VerifiedContext.RunAttempt; profile = $VerifiedContext.Profile; artifact = $VerifiedContext.Artifact; chip_revision = $VerifiedContext.ChipRevision; flash_bytes = $VerifiedContext.FlashBytes; image_digest = $VerifiedContext.ImageDigest; confirmed_at = (Get-Date).ToString('o') } | ConvertTo-Json | Set-Content -LiteralPath $path -Encoding UTF8
}

function Invoke-CheckedFlash([string]$GitExe, [string]$GhExe, [string]$PythonExe, $LaunchContext, $PortContext, $Probe) {
    $temporary = New-SafeTemporaryDirectory
    $fileLocks = @()
    try {
        $downloadOutput = (& $GhExe run download $LaunchContext.RunId --repo $Repo --name $LaunchContext.Artifact --dir $temporary 2>&1 | Out-String)
        if ($LASTEXITCODE -ne 0) { throw "Artifact download failed with exit code $LASTEXITCODE." }
        $packageDir = Find-PackageDirectory $temporary
        $package = Test-PackageManifest $packageDir $LaunchContext.FinalSha $LaunchContext.RunId $LaunchContext.RunAttempt $LaunchContext.Profile
        $fileLocks = @(Lock-VerifiedFlashPlan $package.Plan)
        $finalPortContext = Resolve-PresentP4Port $PortContext.Port
        $finalProbe = Probe-Target $PythonExe $finalPortContext $LaunchContext.Profile
        Assert-TargetUnchanged $PortContext $Probe $finalPortContext $finalProbe
        $finalContext = Assert-LaunchContextUnchanged $LaunchContext $GitExe $GhExe $finalProbe.Profile
        $arguments = @('-m', 'esptool', '--port', $finalPortContext.Port, '--chip', 'esp32p4', '--baud', '921600', 'write_flash', '--flash_mode', [string]$package.Settings.flash_mode, '--flash_freq', [string]$package.Settings.flash_freq, '--flash_size', [string]$package.Settings.flash_size)
        foreach ($entry in $package.Plan) { $arguments += $entry.OffsetText; $arguments += $entry.Path }
        $flashOutput = (& $PythonExe @arguments 2>&1 | Out-String)
        $verifiedCount = ([regex]::Matches($flashOutput, 'Hash of data verified')).Count
        if ($LASTEXITCODE -ne 0 -or $verifiedCount -lt $package.Plan.Count) { throw 'Flashing did not provide successful hash verification for every planned split segment.' }
        return [pscustomobject]@{ FinalSha = $finalContext.FinalSha; Port = $finalPortContext.Port; PnpIdentityHash = $finalPortContext.IdentityHash; RunId = $finalContext.RunId; RunAttempt = $finalContext.RunAttempt; Profile = $finalContext.Profile; Artifact = $finalContext.Artifact; ChipRevision = $finalProbe.Identity.Revision; FlashBytes = $finalProbe.Identity.FlashBytes; ImageDigest = $package.ImageDigest; SegmentCount = $package.Plan.Count; FlashIdOutput = $finalProbe.FlashIdOutput; FlashOutput = $flashOutput }
    }
    finally {
        Close-VerifiedFlashPlanLocks $fileLocks
        Remove-SafeTemporaryDirectory $temporary
    }
}

function Assert-Throws([scriptblock]$Action, [string]$Name) { try { & $Action } catch { return }; throw "SelfTest expected rejection: $Name" }
function Write-SelfTestChecksums([string]$Directory, $Files) { $checksums = @(); foreach ($file in $Files) { $checksums += ('{0} *{1}' -f (Get-FileSha256 (Join-Path $Directory ($file.Split('/') -join [System.IO.Path]::DirectorySeparatorChar))), $file) }; [System.IO.File]::WriteAllLines((Join-Path $Directory 'SHA256SUMS'), [string[]]$checksums) }
function Write-SelfTestManifest([string]$Directory, $Manifest) { $Manifest | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $Directory 'manifest.json') -Encoding UTF8 }
function New-SelfTestPackage([string]$Directory, [string]$ProfileName) {
    New-Item -ItemType Directory -Path (Join-Path $Directory 'bin') | Out-Null
    foreach ($name in @('bin/00002000_bootloader.bin', 'bin/00008000_partition-table.bin', 'bin/00010000_app.bin')) { [System.IO.File]::WriteAllBytes((Join-Path $Directory ($name.Split('/') -join [System.IO.Path]::DirectorySeparatorChar)), @(1,2,3,4)) }
    $mergedPath = Join-Path $Directory 'merged.bin'; $stream = [System.IO.File]::Open($mergedPath, [System.IO.FileMode]::CreateNew, [System.IO.FileAccess]::Write); try { $stream.SetLength($FlashSizeBytes); $stream.WriteByte(1) } finally { $stream.Dispose() }
    $files = @('bin/00002000_bootloader.bin', 'bin/00008000_partition-table.bin', 'bin/00010000_app.bin', 'merged.bin'); Write-SelfTestChecksums $Directory $files
    $manifest = [ordered]@{ schema_version = 2; profile = $ProfileName; target = 'esp32p4'; source_sha = ('a' * 40); pr_head_sha = ('a' * 40); event = 'pull_request'; run_id = '42'; run_attempt = '3'; idf_version = 'v5.5.5'; capacity_bytes = $FlashSizeBytes; capacity = '32MiB'; revision_contract = @{ profile = $ProfileName; selects_rev_less_v3 = $(if ($ProfileName -eq 'rev1_3') { 'y' } else { 'n_or_unset' }); revision_minimum = $(if ($ProfileName -eq 'rev1_3') { '100' } else { '300' }); opposite_minimum_selected = $false }; config_evidence = @{ CONFIG_ESPTOOLPY_FLASHSIZE_32MB = 'y'; CONFIG_ESP32P4_SELECTS_REV_LESS_V3 = $(if ($ProfileName -eq 'rev1_3') { 'y' } else { '<unset>' }); CONFIG_ESP32P4_REV_MIN_100 = $(if ($ProfileName -eq 'rev1_3') { 'y' } else { '<unset>' }); CONFIG_ESP32P4_REV_MIN_300 = $(if ($ProfileName -eq 'rev1_3') { '<unset>' } else { 'y' }) }; c6_binary_included = $false; erase_required = $false; flash_by_default = 'split_files'; flash_files = @(); merged_image = @{ path = 'merged.bin'; size = $FlashSizeBytes; sha256 = (Get-FileSha256 $mergedPath); flash_by_default = $false }; retention_days = 14 }
    foreach ($item in @(@('0x2000', 'bin/00002000_bootloader.bin'), @('0x8000', 'bin/00008000_partition-table.bin'), @('0x10000', 'bin/00010000_app.bin'))) { $path = $item[1]; $full = Join-Path $Directory ($path.Split('/') -join [System.IO.Path]::DirectorySeparatorChar); $manifest.flash_files += @{ offset = $item[0]; path = $path; size = (Get-Item -LiteralPath $full).Length; sha256 = (Get-FileSha256 $full) } }
    Write-SelfTestManifest $Directory $manifest
    @{ flash_files = @{ '0x2000' = 'bin/00002000_bootloader.bin'; '0x8000' = 'bin/00008000_partition-table.bin'; '0x10000' = 'bin/00010000_app.bin' }; flash_settings = @{ flash_mode = 'dio'; flash_freq = '80m'; flash_size = '32MB' }; target = 'esp32p4' } | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $Directory 'flasher_args.json') -Encoding UTF8
    foreach ($name in @('flash_args', 'flash.sh', 'flash.bat')) { Set-Content -LiteralPath (Join-Path $Directory $name) -Value 'esptool.py --chip esp32p4 write_flash 0x2000 bin/00002000_bootloader.bin' -Encoding ASCII }
    return $manifest
}
function Invoke-SelfTest {
    $temporary = New-SafeTemporaryDirectory
    try {
        if ((Test-HeadSha 'bad') -or -not (Test-HeadSha ('a' * 40)) -or (Test-Port 'COM0') -or -not (Test-Port 'COM12') -or (Test-RelativePackagePath $temporary '../escape.bin') -or (Test-RelativePackagePath $temporary 'C:\escape.bin') -or -not (Test-RelativePackagePath $temporary 'bin/app.bin')) { throw 'SelfTest primitive validation failed.' }
        foreach ($profileName in $Profiles) { $directory = Join-Path $temporary $profileName; New-Item -ItemType Directory -Path $directory | Out-Null; $manifest = New-SelfTestPackage $directory $profileName; $package = Test-PackageManifest $directory ('a' * 40) '42' '3' $profileName; if ($package.Plan.Count -ne 3 -or [string]::IsNullOrWhiteSpace($package.ImageDigest)) { throw 'SelfTest profile validation failed.' } }
        $lockPackage = Test-PackageManifest (Join-Path $temporary 'rev3_x') ('a' * 40) '42' '3' 'rev3_x'; $lockedPath = [string]$lockPackage.Plan[0].Path; $replacementPath = Join-Path $temporary 'lock-replacement.bin'; $backupPath = Join-Path $temporary 'lock-backup.bin'; [System.IO.File]::WriteAllBytes($replacementPath, @(1,2,3,4)); $runningOnWindows = [System.Environment]::OSVersion.Platform -eq [System.PlatformID]::Win32NT; $fileLocks = @()
        try {
            $fileLocks = @(Lock-VerifiedFlashPlan $lockPackage.Plan)
            if ($fileLocks.Count -ne $lockPackage.Plan.Count) { throw 'SelfTest did not retain every split-file lock.' }
            Assert-Throws { $writeStream = [System.IO.File]::Open($lockedPath, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Write, [System.IO.FileShare]::None); try { } finally { if ($null -ne $writeStream) { $writeStream.Dispose() } } } 'locked-write'
            if ($runningOnWindows) {
                Assert-Throws { [System.IO.File]::Delete($lockedPath) } 'locked-delete'
                Assert-Throws { [System.IO.File]::Replace($replacementPath, $lockedPath, $backupPath) } 'locked-replace'
            }
        }
        finally { Close-VerifiedFlashPlanLocks $fileLocks }
        $writeStream = [System.IO.File]::Open($lockedPath, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Write, [System.IO.FileShare]::None); $writeStream.Dispose()
        if ($runningOnWindows) {
            [System.IO.File]::Replace($replacementPath, $lockedPath, $backupPath); [System.IO.File]::Delete($lockedPath)
            if (Test-Path -LiteralPath $lockedPath) { throw 'SelfTest lock release did not permit replacement and deletion.' }
        }
        $rev1 = Get-P4FlashIdentity "Chip is ESP32-P4 (revision v1.0)`nDetected flash size: 32MB"; $rev3 = Get-P4FlashIdentity "Chip is ESP32-P4 (revision v3.0)`nDetected flash size: 64MB"
        if ((Resolve-ProfileForIdentity $rev1 'auto') -ne 'rev1_3' -or (Resolve-ProfileForIdentity $rev3 'auto') -ne 'rev3_x') { throw 'SelfTest revision mapping failed.' }
        Assert-Throws { Resolve-ProfileForIdentity $rev1 'rev3_x' } 'profile-mismatch'; Assert-Throws { Get-P4FlashIdentity "Chip is ESP32-P4 (revision v0.0)`nDetected flash size: 32MB" } 'revision-lt-1'; Assert-Throws { Get-P4FlashIdentity "Chip is ESP32-P4 (revision v2.0)`nDetected flash size: 16MB" } 'flash-lt-32'
        $lockedPort = [pscustomobject]@{ Port = 'COM12'; IdentityHash = 'locked-pnp' }; $lockedProbe = [pscustomobject]@{ Identity = $rev1; Profile = 'rev1_3' }; Assert-TargetUnchanged $lockedPort $lockedProbe ([pscustomobject]@{ Port = 'COM12'; IdentityHash = 'locked-pnp' }) ([pscustomobject]@{ Identity = $rev1; Profile = 'rev1_3' }); Assert-Throws { Assert-TargetUnchanged $lockedPort $lockedProbe ([pscustomobject]@{ Port = 'COM13'; IdentityHash = 'locked-pnp' }) ([pscustomobject]@{ Identity = $rev1; Profile = 'rev1_3' }) } 'port-changed'; Assert-Throws { Assert-TargetUnchanged $lockedPort $lockedProbe ([pscustomobject]@{ Port = 'COM12'; IdentityHash = 'changed-pnp' }) ([pscustomobject]@{ Identity = $rev1; Profile = 'rev1_3' }) } 'pnp-changed'; Assert-Throws { Assert-TargetUnchanged $lockedPort $lockedProbe ([pscustomobject]@{ Port = 'COM12'; IdentityHash = 'locked-pnp' }) ([pscustomobject]@{ Identity = $rev3; Profile = 'rev3_x' }) } 'probe-changed'
        $bad = Join-Path $temporary 'rev1_3'; $manifest = Get-Content -LiteralPath (Join-Path $bad 'manifest.json') -Raw | ConvertFrom-Json; $manifest.c6_binary_included = $true; Write-SelfTestManifest $bad $manifest; Assert-Throws { Test-PackageManifest $bad ('a' * 40) '42' '3' 'rev1_3' } 'c6'; $manifest.c6_binary_included = $false; $manifest.erase_required = $true; Write-SelfTestManifest $bad $manifest; Assert-Throws { Test-PackageManifest $bad ('a' * 40) '42' '3' 'rev1_3' } 'erase'; $manifest.erase_required = $false; $manifest.flash_files[1].offset = '0x2000'; Write-SelfTestManifest $bad $manifest; Assert-Throws { Test-PackageManifest $bad ('a' * 40) '42' '3' 'rev1_3' } 'overlap'; $manifest.flash_files[1].offset = '0x8000'; $manifest.flash_files[2].offset = '0x1fffffff'; Write-SelfTestManifest $bad $manifest; Assert-Throws { Test-PackageManifest $bad ('a' * 40) '42' '3' 'rev1_3' } 'out-of-bounds'; $manifest.flash_files[2].offset = '0x10000'; $manifest.source_sha = ('b' * 40); Write-SelfTestManifest $bad $manifest; Assert-Throws { Test-PackageManifest $bad ('a' * 40) '42' '3' 'rev1_3' } 'identity'; $manifest.source_sha = ('a' * 40); Write-SelfTestManifest $bad $manifest; [System.IO.File]::WriteAllBytes((Join-Path $bad 'esp32c6.bin'), @(1)); Assert-Throws { Test-PackageManifest $bad ('a' * 40) '42' '3' 'rev1_3' } 'c6-path'
        Write-Output 'SELF_TEST_OK checks=profiles,manifest-v2,identity,revision-map,profile-mismatch,locked-target,32MiB,c6,no-erase,sha256,overlap,bounds,path-containment segments=3'
    }
    finally { Remove-SafeTemporaryDirectory $temporary }
}

if ($SelfTest) { Invoke-SelfTest; return }
if ($ListOnly) {
    Write-Output ('workflow={0}' -f $Workflow); Write-Output ('profiles=auto,rev1_3,rev3_x; requested={0}' -f $Profile)
    foreach ($profileName in $Profiles) { Write-Output ('artifact={0}{1}-<40-hex-HEAD>' -f $ArtifactPrefix, $profileName) }
    Write-Output 'target=esp32p4; idf_version=v5.5.5; capacity=32MiB/33554432; retention_days=14'
    Write-Output 'revision_map=1.x-2.x:rev1_3,3.x-and-later:rev3_x; revision-lt-1.0=reject; flash-lt-32MiB=reject; explicit-profile-mismatch=reject'
    Write-Output 'safety=tracked-worktree-clean,neutral-branch,one-open-nondraft-pr-exact-head,successful-pull_request-run-exact-head-and-attempt,manifest-v2-and-checksums,present-VID_303A-binding,confirm-and-revalidate'
    Write-Output 'flash_boundary=split_files-only,write_flash-only,merged.bin-not-default,erase-and-C6-rejected'
    Write-Output 'board_warning=Silicon revision does not prove PCB or electrical revision; confirm the intended board design before flashing.'
    return
}

$GitExe = Resolve-Executable 'git'; $GhExe = Resolve-Executable 'gh'; $PythonExe = Resolve-Executable 'python'
$initialPort = Resolve-PresentP4Port $Port
$initialProbe = Probe-Target $PythonExe $initialPort $Profile
$LaunchContext = Get-LaunchContext $GitExe $GhExe $initialProbe.Profile

Add-Type -AssemblyName System.Windows.Forms; Add-Type -AssemblyName System.Drawing
$history = Read-HistoricalState $LaunchContext.FinalSha
$form = New-Object System.Windows.Forms.Form; $form.Text = 'P4 Product Firmware Flasher'; $form.StartPosition = 'CenterScreen'; $form.ClientSize = New-Object System.Drawing.Size(900, 515); $form.FormBorderStyle = 'FixedDialog'; $form.MaximizeBox = $false
function Add-Label([string]$Text, [int]$X, [int]$Y, [int]$Width) { $label = New-Object System.Windows.Forms.Label; $label.Text = $Text; $label.Location = New-Object System.Drawing.Point($X, $Y); $label.Size = New-Object System.Drawing.Size($Width, 22); $form.Controls.Add($label); return $label }
[void](Add-Label "HEAD: $($LaunchContext.FinalSha)" 15 15 870); [void](Add-Label "PR: #$($LaunchContext.PullRequestNumber)   Run: $($LaunchContext.RunId) attempt $($LaunchContext.RunAttempt)   Profile: $($LaunchContext.Profile)" 15 42 870); [void](Add-Label "Artifact: $($LaunchContext.Artifact)" 15 69 870)
[void](Add-Label "Probed target: ESP32-P4 v$($initialProbe.Identity.Revision), $([int]($initialProbe.Identity.FlashBytes / 1MB)) MiB flash." 15 96 870)
[void](Add-Label 'Warning: silicon revision does not prove PCB or electrical revision. Confirm this is the intended board design before flashing.' 15 123 870)
[void](Add-Label 'Only checksummed split files are written with write_flash; C6 content, erase commands, and merged.bin-as-default are rejected.' 15 150 870)
[void](Add-Label 'Port:' 15 183 45); $portBox = New-Object System.Windows.Forms.TextBox; $portBox.Text = $initialPort.Port; $portBox.Location = New-Object System.Drawing.Point(65, 180); $portBox.Size = New-Object System.Drawing.Size(100, 22); $form.Controls.Add($portBox)
$statusLabel = Add-Label 'Status: Ready. Click Flash to confirm, revalidate context and target, then download and verify the exact CI artifact.' 15 218 870
$outputBox = New-Object System.Windows.Forms.TextBox; $outputBox.Multiline = $true; $outputBox.ReadOnly = $true; $outputBox.ScrollBars = 'Both'; $outputBox.WordWrap = $false; $outputBox.Font = New-Object System.Drawing.Font('Consolas', 9); $outputBox.Location = New-Object System.Drawing.Point(15, 248); $outputBox.Size = New-Object System.Drawing.Size(870, 190); $form.Controls.Add($outputBox)
$flashButton = New-Object System.Windows.Forms.Button; $flashButton.Text = 'Flash verified split files'; $flashButton.Location = New-Object System.Drawing.Point(15, 455); $flashButton.Size = New-Object System.Drawing.Size(190, 32); $form.Controls.Add($flashButton)
$passButton = New-Object System.Windows.Forms.Button; $passButton.Text = 'Mark hardware PASS'; $passButton.Location = New-Object System.Drawing.Point(215, 455); $passButton.Size = New-Object System.Drawing.Size(165, 32); $passButton.Enabled = $false; $form.Controls.Add($passButton)
$exitButton = New-Object System.Windows.Forms.Button; $exitButton.Text = 'Exit'; $exitButton.Location = New-Object System.Drawing.Point(765, 455); $exitButton.Size = New-Object System.Drawing.Size(120, 32); $form.Controls.Add($exitButton)
$script:VerifiedFlashContext = $null
if ($null -ne $history) { $statusLabel.Text = 'Status: A historical record exists for this HEAD. Reflash and verify the current target before it can be treated as hardware PASS.' }
function Set-Busy([bool]$Busy) { $flashButton.Enabled = -not $Busy; $passButton.Enabled = (-not $Busy) -and ($null -ne $script:VerifiedFlashContext); $exitButton.Enabled = -not $Busy; $portBox.Enabled = (-not $Busy) -and ($null -eq $script:VerifiedFlashContext); $form.UseWaitCursor = $Busy; [System.Windows.Forms.Application]::DoEvents() }
$flashButton.Add_Click({
    $selectedPort = $portBox.Text.Trim().ToUpperInvariant(); if (-not (Test-Port $selectedPort)) { [System.Windows.Forms.MessageBox]::Show('Port must be COM followed by a positive number, for example COM12.', 'Invalid port') | Out-Null; return }
    $confirmation = [System.Windows.Forms.MessageBox]::Show('Confirm the intended PCB/electrical board design. The tool will re-probe the target and write only verified split files. Flash now?', 'Confirm flashing', [System.Windows.Forms.MessageBoxButtons]::YesNo, [System.Windows.Forms.MessageBoxIcon]::Warning)
    if ($confirmation -ne [System.Windows.Forms.DialogResult]::Yes) { return }
    $script:VerifiedFlashContext = $null; Set-Busy $true
    try {
        $portContext = Resolve-PresentP4Port $selectedPort; $probe = Probe-Target $PythonExe $portContext $Profile
        $currentContext = Assert-LaunchContextUnchanged $LaunchContext $GitExe $GhExe $probe.Profile
        $result = Invoke-CheckedFlash $GitExe $GhExe $PythonExe $currentContext $portContext $probe
        $outputBox.Text = "flash_id:`r`n$($result.FlashIdOutput)`r`n`r`nwrite_flash:`r`n$($result.FlashOutput)"; $script:VerifiedFlashContext = $result
        $statusLabel.Text = "Status: Split-file hash verification passed for $($result.SegmentCount) segments. Target v$($result.ChipRevision), $([int]($result.FlashBytes / 1MB)) MiB. Perform hardware checks before marking PASS."
    } catch { $outputBox.Text = ($_ | Out-String); $statusLabel.Text = "Status: Flashing failed. $($_.Exception.Message)" } finally { Set-Busy $false }
})
$passButton.Add_Click({ if ($null -eq $script:VerifiedFlashContext) { return }; $confirmation = [System.Windows.Forms.MessageBox]::Show('Confirm the intended board runs correctly and there is no panic or reset. This records hardware PASS only for this immutable verified context.', 'Confirm hardware PASS', [System.Windows.Forms.MessageBoxButtons]::YesNo, [System.Windows.Forms.MessageBoxIcon]::Question); if ($confirmation -ne [System.Windows.Forms.DialogResult]::Yes) { return }; Save-HardwarePass $script:VerifiedFlashContext; $statusLabel.Text = 'Status: Hardware PASS recorded for the immutable verified flash context.'; Set-Busy $false })
$exitButton.Add_Click({ $form.Close() })
[void]$form.ShowDialog()
