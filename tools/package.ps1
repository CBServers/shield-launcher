[CmdletBinding()]
param(
    [string]$Configuration = 'Release',
    [string]$BuildDir = 'build/bin/x64',
    [string]$ReleaseDir = 'project-bo4/Latest-Release-Files'
)

$ErrorActionPreference = 'Stop'

$repoRoot    = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$srcDir      = Join-Path $repoRoot (Join-Path $BuildDir $Configuration)
$dstRoot     = Join-Path $repoRoot $ReleaseDir
$dstLauncher = Join-Path $dstRoot 'project-bo4/launcher'

if (-not (Test-Path -LiteralPath $srcDir)) {
    throw "Build output not found at $srcDir. Build $Configuration|x64 before packaging."
}
if (-not (Test-Path -LiteralPath $dstLauncher)) {
    throw "Release tree not found at $dstLauncher. Is the checkout complete?"
}

Write-Host "Packaging $Configuration build"
Write-Host "  source: $srcDir"
Write-Host "  target: $dstRoot"

function Copy-Required {
    param([string]$Name, [string]$DstDir)
    $src = Join-Path $srcDir $Name
    if (-not (Test-Path -LiteralPath $src)) {
        throw "Missing required build output: $src"
    }
    Copy-Item -LiteralPath $src -Destination (Join-Path $DstDir $Name) -Force
    Write-Host "  copied $Name"
}

# Launcher binaries + Qt runtime DLLs
$launcherFiles = @(
    'Shield_Launcher.exe',
    'Qt6Core.dll',
    'Qt6Gui.dll',
    'Qt6Network.dll',
    'Qt6Widgets.dll'
)
foreach ($f in $launcherFiles) {
    Copy-Required -Name $f -DstDir $dstLauncher
}

# Qt platform plugin folder (replace wholesale)
$srcPlatforms = Join-Path $srcDir 'platforms'
$dstPlatforms = Join-Path $dstLauncher 'platforms'
if (-not (Test-Path -LiteralPath $srcPlatforms)) {
    throw "Missing required build output: $srcPlatforms"
}
if (Test-Path -LiteralPath $dstPlatforms) {
    Remove-Item -LiteralPath $dstPlatforms -Recurse -Force
}
Copy-Item -LiteralPath $srcPlatforms -Destination $dstPlatforms -Recurse -Force
Write-Host "  copied platforms/"

# Top-level shortcut
Copy-Required -Name 'Launch Project BO4.exe' -DstDir $dstRoot

# Migrate legacy mp.zip/solo.zip into mp/ and solo/ folders so the launcher can copy the DLL directly
foreach ($mode in 'mp','solo') {
    $zip    = Join-Path $dstLauncher "$mode.zip"
    $target = Join-Path $dstLauncher $mode
    if (Test-Path -LiteralPath $zip) {
        if (-not (Test-Path -LiteralPath $target)) {
            New-Item -ItemType Directory -Path $target | Out-Null
        }
        Expand-Archive -LiteralPath $zip -DestinationPath $target -Force
        Remove-Item -LiteralPath $zip -Force
        Write-Host "  migrated $mode.zip -> $mode/"
    }
}

# Strip any stale update zips at the release root so the artifact is clean
$staleZips = Get-ChildItem -LiteralPath $dstRoot -Filter '*.zip' -File -ErrorAction SilentlyContinue
foreach ($z in $staleZips) {
    Remove-Item -LiteralPath $z.FullName -Force
    Write-Host "  removed stale $($z.Name)"
}

Write-Host "Package complete: $dstRoot"
