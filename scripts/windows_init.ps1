# PowerShell script to configure CMake build for Windows (MSVC/Visual Studio)
param(
    [ValidateSet("debug", "release")]
    [string]$BuildType = "debug",
    [string]$Architecture = "x64",
    [string]$VisualStudioVersion = ""
)

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RootDir = Split-Path -Parent $ScriptDir
$Preset = "user-$BuildType-windows"
$BuildDir = Join-Path $RootDir "cmake-build-$BuildType"

# Check if preset exists. If not, run generate_user_presets.ps1
$PresetsFile = Join-Path $RootDir "CMakeUserPresets.json"
if (-not (Test-Path $PresetsFile)) {
    Write-Host "CMakeUserPresets.json not found. Generating from template..."
    & (Join-Path $ScriptDir "generate_user_presets.ps1")
}

# Detect Visual Studio version if not specified
if (-not $VisualStudioVersion) {
    # Try to find latest Visual Studio using vswhere
    $VswherePath = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $VswherePath) {
        $VsVersion = & $VswherePath -latest -property installationVersion 2>$null
        if ($VsVersion) {
            $VsYear = ($VsVersion -split '\.')[0]
            switch ($VsYear) {
                "17" { $VisualStudioVersion = "Visual Studio 17 2022" }
                "16" { $VisualStudioVersion = "Visual Studio 16 2019" }
                "15" { $VisualStudioVersion = "Visual Studio 15 2017" }
                default { $VisualStudioVersion = "Visual Studio 17 2022" }
            }
            Write-Host "Detected Visual Studio version: $VisualStudioVersion"
        } else {
            $VisualStudioVersion = "Visual Studio 17 2022"
            Write-Host "Could not detect Visual Studio version, defaulting to: $VisualStudioVersion"
        }
    } else {
        $VisualStudioVersion = "Visual Studio 17 2022"
        Write-Host "vswhere not found, defaulting to: $VisualStudioVersion"
    }
}

# Configure CMake with Visual Studio generator
# Note: Visual Studio generators are multi-config, so CMAKE_BUILD_TYPE is ignored
# Build type is specified at build time with: cmake --build . --config Debug/Release
cmake --preset $Preset `
    -G $VisualStudioVersion `
    -A $Architecture `
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

Write-Host "Configured $BuildType MSVC build in: $BuildDir"
Write-Host "Preset: $Preset"
Write-Host "Generator: $VisualStudioVersion"
Write-Host "Architecture: $Architecture"
