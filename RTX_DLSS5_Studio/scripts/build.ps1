#----------------------------------------------------------------------------------
# File:        build.ps1
# Project:     RTX DLSS-5 Video Studio
# Purpose:     Build runner for Visual Studio 2026 / 2022 MSBuild
#----------------------------------------------------------------------------------

param(
    [string]$Configuration = "Release",
    [string]$Platform = "x64"
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectDir = Resolve-Path "$scriptDir\.."
$solutionPath = "$projectDir\RTX_DLSS5_Studio.sln"

Write-Host "=================================================================" -ForegroundColor Cyan
Write-Host " Building RTX DLSS-5 Video Studio [$Configuration|$Platform]" -ForegroundColor Cyan
Write-Host "=================================================================" -ForegroundColor Cyan

# Locate MSBuild
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {
    throw "Visual Studio Installer not found."
}

$vsPath = & $vswhere -latest -products * -property installationPath
$msbuild = "$vsPath\MSBuild\Current\Bin\MSBuild.exe"

if (-not (Test-Path $msbuild)) {
    throw "MSBuild.exe not found at $msbuild"
}

# Set environment variable for SDK
$sdkRoot = (Resolve-Path "$projectDir\..").Path
$env:NV_RTX_VIDEO_SDK = $sdkRoot

# Ensure 64-bit MSVC toolchain is used to prevent MSPDB140.DLL architecture mismatch
$msvcBin = Get-ChildItem "C:\Program Files (x86)\Microsoft Visual Studio\*\BuildTools\VC\Tools\MSVC\*\bin\Hostx64\x64" -ErrorAction SilentlyContinue | Select-Object -First 1
if ($msvcBin) {
    $env:PATH = "$($msvcBin.FullName);$env:PATH"
}

Write-Host "Using MSBuild: $msbuild" -ForegroundColor Gray
Write-Host "SDK Root: $sdkRoot" -ForegroundColor Gray

# Execute MSBuild
$buildArgs = @(
    "$solutionPath",
    "/p:Configuration=$Configuration",
    "/p:Platform=$Platform",
    "/p:PreferredToolArchitecture=x64",
    "/m",
    "/v:minimal"
)

& $msbuild @buildArgs

if ($LASTEXITCODE -eq 0) {
    Write-Host "`n[SUCCESS] Build completed successfully!" -ForegroundColor Green
    $outputExe = "$projectDir\bin\$Platform\$Configuration\RTX_DLSS5_Studio.exe"
    if (Test-Path $outputExe) {
        Write-Host "Executable generated: $outputExe" -ForegroundColor Green
    }
} else {
    Write-Host "`n[FAILED] Build failed with exit code $LASTEXITCODE" -ForegroundColor Red
}
