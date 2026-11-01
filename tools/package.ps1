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

# Launcher binary (the UI is built into it and the WebView2 loader is linked statically)
Copy-Required -Name 'Shield_Launcher.exe' -DstDir $dstLauncher

# Top-level shortcut
Copy-Required -Name 'Launch Project BO4.exe' -DstDir $dstRoot

# Strip any stale update zips at the release root so the artifact is clean
$staleZips = Get-ChildItem -LiteralPath $dstRoot -Filter '*.zip' -File -ErrorAction SilentlyContinue
foreach ($z in $staleZips) {
    Remove-Item -LiteralPath $z.FullName -Force
    Write-Host "  removed stale $($z.Name)"
}

Write-Host "Package complete: $dstRoot"
