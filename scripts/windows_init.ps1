# PowerShell script to configure CMake build for Windows (Ninja)
param(
    [ValidateSet("debug", "release")]
    [string]$BuildType = "debug"
)

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RootDir = Split-Path -Parent $ScriptDir
$Preset = "user-$BuildType"
$BuildDir = Join-Path $RootDir "cmake-build-$BuildType"

# Check if preset exists. If not, run generate_user_presets.ps1
$PresetsFile = Join-Path $RootDir "CMakeUserPresets.json"
if (-not (Test-Path $PresetsFile)) {
    Write-Host "CMakeUserPresets.json not found. Generating from template..."
    & (Join-Path $ScriptDir "generate_user_presets.ps1")
}

# Configure CMake
cmake --preset $Preset `
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

Write-Host "Configured $BuildType Ninja build in: $BuildDir"
Write-Host "Preset: $Preset"
