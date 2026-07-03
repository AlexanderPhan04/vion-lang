param(
    [string]$Repo = "AlexanderPhan04/vion-lang",
    [string]$Version = "latest",
    [string]$InstallDir = "",
    [string]$AssetPattern = "vion-*-Windows.zip",
    [string]$ZipUrl = "",
    [switch]$NoPath
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($InstallDir)) {
    $InstallDir = Join-Path $env:LOCALAPPDATA "Programs\Vion"
}

$BinDir = Join-Path $InstallDir "bin"
$DocsDir = Join-Path $InstallDir "docs"
$ExamplesDir = Join-Path $InstallDir "examples"
$InstallerUserAgent = "VionOnlineInstaller"

function Add-UserPathEntry {
    param([string]$PathEntry)

    $ResolvedEntry = [System.IO.Path]::GetFullPath($PathEntry).TrimEnd('\')
    $CurrentPath = [Environment]::GetEnvironmentVariable("Path", "User")

    if ([string]::IsNullOrWhiteSpace($CurrentPath)) {
        $Entries = @()
    } else {
        $Entries = $CurrentPath -split ";" | Where-Object { ![string]::IsNullOrWhiteSpace($_) }
    }

    foreach ($Entry in $Entries) {
        $ExpandedEntry = [Environment]::ExpandEnvironmentVariables($Entry)
        if ([System.IO.Path]::GetFullPath($ExpandedEntry).TrimEnd('\').Equals($ResolvedEntry, [System.StringComparison]::OrdinalIgnoreCase)) {
            Write-Host "PATH already contains $ResolvedEntry"
            return
        }
    }

    $NewPath = (($Entries + $ResolvedEntry) -join ";")
    [Environment]::SetEnvironmentVariable("Path", $NewPath, "User")
    $env:Path = $env:Path + ";" + $ResolvedEntry

    Write-Host "Added to user PATH: $ResolvedEntry"
    Write-Host "Open a new terminal before running vion from anywhere."
}

function Get-ReleaseAssetUrl {
    param(
        [string]$Repository,
        [string]$ReleaseVersion,
        [string]$Pattern
    )

    if ($ReleaseVersion -eq "latest") {
        $ReleaseApiUrl = "https://api.github.com/repos/$Repository/releases/latest"
    } else {
        $ReleaseApiUrl = "https://api.github.com/repos/$Repository/releases/tags/$ReleaseVersion"
    }

    Write-Host "Checking GitHub release: $ReleaseApiUrl"
    $Release = Invoke-RestMethod -Uri $ReleaseApiUrl -Headers @{ "User-Agent" = $InstallerUserAgent }
    $Assets = @($Release.assets)
    $Asset = $Assets | Where-Object { $_.name -like $Pattern } | Select-Object -First 1

    if ($null -eq $Asset) {
        $AvailableAssets = ($Assets | ForEach-Object { $_.name }) -join ", "
        throw "Could not find release asset matching '$Pattern'. Available assets: $AvailableAssets"
    }

    Write-Host "Selected release asset: $($Asset.name)"
    return $Asset.browser_download_url
}

function Copy-FirstMatchingFile {
    param(
        [string]$Root,
        [string]$FileName,
        [string]$Destination
    )

    $File = Get-ChildItem -LiteralPath $Root -Recurse -File -Filter $FileName | Select-Object -First 1
    if ($null -ne $File) {
        Copy-Item -LiteralPath $File.FullName -Destination $Destination -Force
    }
}

$TempRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("vion-install-" + [System.Guid]::NewGuid().ToString("N"))
$ZipPath = Join-Path $TempRoot "vion.zip"
$ExtractDir = Join-Path $TempRoot "extract"

try {
    New-Item -ItemType Directory -Force -Path $TempRoot | Out-Null
    New-Item -ItemType Directory -Force -Path $ExtractDir | Out-Null

    if ([string]::IsNullOrWhiteSpace($ZipUrl)) {
        $ZipUrl = Get-ReleaseAssetUrl -Repository $Repo -ReleaseVersion $Version -Pattern $AssetPattern
    }

    if (Test-Path -LiteralPath $ZipUrl) {
        Write-Host "Using local package $ZipUrl"
        Copy-Item -LiteralPath $ZipUrl -Destination $ZipPath -Force
    } else {
        Write-Host "Downloading Vion from $ZipUrl"
        Invoke-WebRequest -Uri $ZipUrl -OutFile $ZipPath -UseBasicParsing -Headers @{ "User-Agent" = $InstallerUserAgent }
    }

    Write-Host "Extracting package"
    Expand-Archive -LiteralPath $ZipPath -DestinationPath $ExtractDir -Force

    $VionExe = Get-ChildItem -LiteralPath $ExtractDir -Recurse -File -Filter "vion.exe" | Select-Object -First 1
    if ($null -eq $VionExe) {
        throw "The downloaded package did not contain vion.exe."
    }

    New-Item -ItemType Directory -Force -Path $BinDir | Out-Null
    New-Item -ItemType Directory -Force -Path $DocsDir | Out-Null
    New-Item -ItemType Directory -Force -Path $ExamplesDir | Out-Null

    Copy-Item -LiteralPath $VionExe.FullName -Destination (Join-Path $BinDir "vion.exe") -Force

    foreach ($DocName in @("LICENSE", "README.md", "INSTALL.md", "LANGUAGE_SPEC.md", "ROADMAP.md")) {
        Copy-FirstMatchingFile -Root $ExtractDir -FileName $DocName -Destination $DocsDir
    }

    $ExampleFiles = Get-ChildItem -LiteralPath $ExtractDir -Recurse -File -Filter "*.vion" |
        Where-Object { $_.FullName -match "[/\\]examples[/\\]" }

    foreach ($ExampleFile in $ExampleFiles) {
        Copy-Item -LiteralPath $ExampleFile.FullName -Destination $ExamplesDir -Force
    }

    if (!$NoPath) {
        Add-UserPathEntry -PathEntry $BinDir
    }

    Write-Host "Installed Vion to $InstallDir"
    & (Join-Path $BinDir "vion.exe") version
    Write-Host "Try: vion $ExamplesDir\hello.vion"
} finally {
    if (Test-Path $TempRoot) {
        Remove-Item -LiteralPath $TempRoot -Recurse -Force
    }
}
