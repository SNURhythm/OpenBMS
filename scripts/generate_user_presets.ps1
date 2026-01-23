# PowerShell script to generate CMakeUserPresets.json from template
param(
    [string]$VcpkgPath = ""
)

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RootDir = Split-Path -Parent $ScriptDir
$TemplateFile = Join-Path $RootDir "CMakeUserPresetsTemplate.json"
$OutputFile = Join-Path $RootDir "CMakeUserPresets.json"

# Determine VCPKG_ROOT
if ($VcpkgPath) {
    $VcpkgPath = $VcpkgPath
} elseif ($env:VCPKG_ROOT) {
    $VcpkgPath = $env:VCPKG_ROOT
} elseif (Test-Path "$env:USERPROFILE\vcpkg") {
    $VcpkgPath = "$env:USERPROFILE\vcpkg"
} elseif (Test-Path "$RootDir\vcpkg") {
    $VcpkgPath = "$RootDir\vcpkg"
} else {
    Write-Host "VCPKG_ROOT not found. Please set VCPKG_ROOT environment variable or install vcpkg in $env:USERPROFILE\vcpkg"
    $InputPath = Read-Host "Enter vcpkg path (or press Enter to use $env:USERPROFILE\vcpkg)"
    if ($InputPath) {
        $VcpkgPath = $InputPath
    } else {
        $VcpkgPath = "$env:USERPROFILE\vcpkg"
    }
}

# Normalize path (convert to absolute, handle forward/backward slashes)
$VcpkgPath = (Resolve-Path -Path $VcpkgPath -ErrorAction SilentlyContinue).Path
if (-not $VcpkgPath) {
    # If Resolve-Path fails (path doesn't exist), normalize manually
    $VcpkgPath = $VcpkgPath -replace '/', '\'
    if (-not [System.IO.Path]::IsPathRooted($VcpkgPath)) {
        $VcpkgPath = Join-Path $PWD $VcpkgPath
    }
}

# Check if template exists
if (-not (Test-Path $TemplateFile)) {
    Write-Error "Template file not found: $TemplateFile"
    exit 1
}

# Escape backslashes for JSON (Windows paths need \\ in JSON)
$VcpkgPathEscaped = $VcpkgPath.Replace('\', '\\')

# Read template and replace placeholder
$Content = Get-Content $TemplateFile -Raw
$Content = $Content -replace '\$\{VCPKG_ROOT\}', $VcpkgPathEscaped

# Write output file
$Content | Set-Content $OutputFile -NoNewline

Write-Host "Generated $OutputFile"
Write-Host "VCPKG_ROOT: $VcpkgPath"
