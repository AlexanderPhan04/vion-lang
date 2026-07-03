param(
    [string]$InstallDir = ""
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($InstallDir)) {
    $InstallDir = Join-Path $env:LOCALAPPDATA "Programs\Vion"
}

$BinDir = Join-Path $InstallDir "bin"
$ResolvedBinDir = [System.IO.Path]::GetFullPath($BinDir).TrimEnd('\')
$CurrentPath = [Environment]::GetEnvironmentVariable("Path", "User")

if (![string]::IsNullOrWhiteSpace($CurrentPath)) {
    $Entries = @()

    foreach ($Entry in ($CurrentPath -split ";")) {
        if ([string]::IsNullOrWhiteSpace($Entry)) {
            continue
        }

        $ExpandedEntry = [Environment]::ExpandEnvironmentVariables($Entry)
        if (![System.IO.Path]::GetFullPath($ExpandedEntry).TrimEnd('\').Equals($ResolvedBinDir, [System.StringComparison]::OrdinalIgnoreCase)) {
            $Entries += $Entry
        }
    }

    [Environment]::SetEnvironmentVariable("Path", ($Entries -join ";"), "User")
    Write-Host "Removed from user PATH: $ResolvedBinDir"
}

if (Test-Path $InstallDir) {
    Remove-Item -LiteralPath $InstallDir -Recurse -Force
    Write-Host "Removed Vion installation: $InstallDir"
} else {
    Write-Host "Vion installation was not found: $InstallDir"
}

Write-Host "Open a new terminal to refresh PATH."
