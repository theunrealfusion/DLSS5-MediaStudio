#----------------------------------------------------------------------------------
# File:        setup_env.ps1
# Project:     RTX DLSS-5 Video Studio
# Purpose:     Environment verification, GPU telemetry check, and dependency validator
#----------------------------------------------------------------------------------

$ErrorActionPreference = "Stop"

Write-Host "=================================================================" -ForegroundColor Cyan
Write-Host " RTX DLSS-5 Studio - Environment & System Diagnostics" -ForegroundColor Cyan
Write-Host "=================================================================" -ForegroundColor Cyan

# 1. Check GPU Hardware
Write-Host "`n[1/4] Detecting GPU Hardware & Driver..." -ForegroundColor Yellow
try {
    $smi = nvidia-smi --query-gpu=name,driver_version,memory.total --format=csv,noheader
    Write-Host "  Detected GPU: $smi" -ForegroundColor Green
    if ($smi -match "5090") {
        Write-Host "  [OPTIMAL] NVIDIA GeForce RTX 5090 Blackwell Detected!" -ForegroundColor Green
    }
} catch {
    Write-Host "  [WARNING] nvidia-smi failed. Ensure NVIDIA Graphics Driver (R550+ or R610+) is installed." -ForegroundColor Red
}

# 2. Check NVIDIA RTX Video SDK Runtime Files
Write-Host "`n[2/4] Verifying RTX Video SDK v1.1.0 Binaries..." -ForegroundColor Yellow
$sdkRoot = (Resolve-Path "$PSScriptRoot\..\..").Path
$headers = "$sdkRoot\include\nvsdk_ngx.h"
$vsrDll = "$sdkRoot\bin\Windows\x64\rel\nvngx_vsr.dll"
$hdrDll = "$sdkRoot\bin\Windows\x64\rel\nvngx_truehdr.dll"
$ngxLib = "$sdkRoot\lib\Windows\x64\nvsdk_ngx_s.lib"

if ((Test-Path $headers) -and (Test-Path $vsrDll) -and (Test-Path $hdrDll) -and (Test-Path $ngxLib)) {
    Write-Host "  NVIDIA NGX VSR DLL: PRESENT ($(Get-Item $vsrDll | Select-Object -ExpandProperty Length) bytes)" -ForegroundColor Green
    Write-Host "  NVIDIA NGX TrueHDR DLL: PRESENT ($(Get-Item $hdrDll | Select-Object -ExpandProperty Length) bytes)" -ForegroundColor Green
    Write-Host "  NVIDIA NGX Headers & Libs: VERIFIED" -ForegroundColor Green
} else {
    Write-Host "  [ERROR] Missing core RTX Video SDK files in $sdkRoot" -ForegroundColor Red
}

# 3. Check Visual Studio Build Tools
Write-Host "`n[3/4] Checking Visual Studio C++ Compiler..." -ForegroundColor Yellow
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path $vswhere) {
    $vsPath = & $vswhere -latest -products * -property installationPath
    $vsVer = & $vswhere -latest -products * -property installationVersion
    Write-Host "  Found Visual Studio / Build Tools at: $vsPath ($vsVer)" -ForegroundColor Green
} else {
    Write-Host "  [WARNING] Visual Studio Installer not found." -ForegroundColor Red
}

# 4. Check Windows 11 SDK
Write-Host "`n[4/4] Checking Windows 10/11 SDK..." -ForegroundColor Yellow
$sdkFound = $false
$sdkLocations = @(
    "${env:ProgramFiles(x86)}\Windows Kits\10\Include",
    "${env:ProgramFiles}\Windows Kits\10\Include"
)

foreach ($loc in $sdkLocations) {
    if (Test-Path $loc) {
        $versions = Get-ChildItem $loc | Select-Object -ExpandProperty Name
        if ($versions) {
            Write-Host "  Found Windows SDK Headers: $($versions -join ', ')" -ForegroundColor Green
            $sdkFound = $true
            break
        }
    }
}

if (-not $sdkFound) {
    Write-Host "  [NOTICE] Windows SDK headers not yet detected in default path." -ForegroundColor Yellow
    Write-Host "  Please install 'Windows 11 SDK (10.0.26100.0)' via the Visual Studio Installer" -ForegroundColor Cyan
    Write-Host "  or run the installer setup cached at:" -ForegroundColor Cyan
    Write-Host "  C:\Users\kshvs\AppData\Local\Temp\WinGet\Microsoft.WindowsSDK.10.0.26100.10.0.26100.7705\winsdksetup.exe" -ForegroundColor White
}

Write-Host "`n=================================================================" -ForegroundColor Cyan
Write-Host " Environment Check Completed" -ForegroundColor Cyan
Write-Host "=================================================================" -ForegroundColor Cyan
