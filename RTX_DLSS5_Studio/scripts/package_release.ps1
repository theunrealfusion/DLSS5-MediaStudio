#----------------------------------------------------------------------------------
# File:        package_release.ps1
# Project:     RTX DLSS-5 Video Studio
# Purpose:     Automated release packaging, portable zip bundle, and SHA256 generator
#----------------------------------------------------------------------------------

param(
    [string]$Version = "v1.1.0",
    [switch]$SkipBuild,
    [switch]$Publish,
    [switch]$Draft,
    [switch]$Prerelease,
    [string]$Configuration = "Release",
    [string]$Platform = "x64"
)

$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"
Add-Type -AssemblyName System.IO.Compression.FileSystem

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectDir = Resolve-Path "$scriptDir\.."
$sdkRoot = (Resolve-Path "$projectDir\..").Path
$binDir = "$projectDir\bin\$Platform\$Configuration"
$distDir = "$projectDir\dist"

Write-Host "=================================================================" -ForegroundColor Cyan
Write-Host " RTX DLSS-5 Studio - Release Packaging [$Version]" -ForegroundColor Cyan
Write-Host "=================================================================" -ForegroundColor Cyan

# 1. Build if requested
if (-not $SkipBuild) {
    Write-Host "`n[1/6] Building project in $Configuration|$Platform..." -ForegroundColor Yellow
    & "$scriptDir\build.ps1" -Configuration $Configuration -Platform $Platform
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed with exit code $LASTEXITCODE"
    }
} else {
    Write-Host "`n[1/6] Skipping build step (using existing binaries)..." -ForegroundColor Yellow
}

# 2. Verify all binary assets
Write-Host "`n[2/6] Verifying binary artifacts..." -ForegroundColor Yellow
$exePath = "$binDir\RTX_DLSS5_Studio.exe"
$vsrDllPath = "$binDir\nvngx_vsr.dll"
$hdrDllPath = "$binDir\nvngx_truehdr.dll"
$shaderPath = "$binDir\Shaders.hlsl"

$requiredFiles = @($exePath, $vsrDllPath, $hdrDllPath, $shaderPath)
foreach ($f in $requiredFiles) {
    if (-not (Test-Path $f)) {
        throw "Required artifact missing: $f"
    }
    $size = (Get-Item $f).Length
    Write-Host "  Found: $(Split-Path -Leaf $f) ($size bytes)" -ForegroundColor Green
}

# 3. Setup Dist directory
Write-Host "`n[3/6] Setting up distribution staging directory..." -ForegroundColor Yellow
if (Test-Path $distDir) {
    Remove-Item -Path $distDir -Recurse -Force
}
New-Item -ItemType Directory -Path $distDir -Force | Out-Null

$bundleName = "RTX_DLSS5_Studio-$Version-Windows-x64"
$stageDir = "$distDir\$bundleName"
New-Item -ItemType Directory -Path $stageDir -Force | Out-Null

# Copy binary deliverables
Copy-Item $exePath -Destination $stageDir
Copy-Item $vsrDllPath -Destination $stageDir
Copy-Item $hdrDllPath -Destination $stageDir
Copy-Item $shaderPath -Destination $stageDir

# Standalone EXE in dist root for direct one-click download
Copy-Item $exePath -Destination "$distDir\RTX_DLSS5_Studio.exe"

# 4. Generate End-User Quick-Start README and Launcher
Write-Host "`n[4/6] Creating end-user documentation and launchers..." -ForegroundColor Yellow

$readmeContent = @"
===============================================================================
              NVIDIA RTX DLSS-5 Video Studio [$Version]
        Optimized for NVIDIA GeForce RTX 5090 (Blackwell Architecture)
===============================================================================

Thank you for downloading NVIDIA RTX DLSS-5 Video Studio!
This application provides studio-grade AI video playback, super-resolution (VSR),
TrueHDR conversion, and interactive side-by-side comparison.

-------------------------------------------------------------------------------
1. QUICK START
-------------------------------------------------------------------------------
Option A: Double-click 'run.bat' or 'RTX_DLSS5_Studio.exe' to launch the GUI.
Option B: Drag and drop ANY video file (.mp4, .mkv, .mov, .avi, etc.) directly
          onto the application window or onto the executable icon.
Option C: Press 'O' or 'Ctrl+O' inside the app to open the file selection dialog.

-------------------------------------------------------------------------------
2. KEYBOARD & MOUSE CONTROLS
-------------------------------------------------------------------------------
  [Left Click / Drag]   Slide comparison separator in real time
  [Middle Click / C]    Center split separator to 50%
  [Mouse Wheel]         Smoothly shift comparison wipe
  [D / Tab]             Cycle comparison mode (Split, Side-by-Side, Heatmap, Quad)
  [V]                   Toggle wipe orientation (Vertical vs Horizontal)
  [L]                   Toggle 8x interactive magnifying loupe
  [1] - [4]             Set VSR Quality level (1=Low, 2=Med, 3=High, 4=Ultra)
  [0]                   Bicubic baseline mode (bypass neural upscaler)
  [H]                   Toggle RTX TrueHDR On/Off
  [Space]               Play / Pause playback
  [Left / Right]        Step frame backward / forward
  [+ / -]               Adjust difference heatmap gain
  [F1]                  Toggle real-time telemetry HUD overlay
  [F12]                 Export signed timestamped comparison snapshot
  [Esc]                 Exit application

-------------------------------------------------------------------------------
3. SYSTEM REQUIREMENTS
-------------------------------------------------------------------------------
- Windows 10 (21H2+) or Windows 11 (64-bit)
- NVIDIA GeForce RTX 20, 30, 40, or 50 Series GPU (RTX 5090 Blackwell Recommended)
- NVIDIA Graphics Driver R550.50 or newer (R610+ Recommended)
- Direct3D 12 Feature Level 12.0+

-------------------------------------------------------------------------------
4. COMMAND-LINE USAGE
-------------------------------------------------------------------------------
  RTX_DLSS5_Studio.exe -i "my_video.mp4" -mode split -vsrQuality 4 -thdr
  RTX_DLSS5_Studio.exe -batch "output_upscaled.mp4" -i "input.mp4"
  RTX_DLSS5_Studio.exe -diag
  RTX_DLSS5_Studio.exe -help

===============================================================================
"@

$readmeContent | Out-File -FilePath "$stageDir\README.txt" -Encoding ascii
$readmeContent | Out-File -FilePath "$distDir\README.txt" -Encoding ascii

$runBatContent = @"
@echo off
cd /d "%~dp0"
start "" "%~dp0RTX_DLSS5_Studio.exe" %*
"@
$runBatContent | Out-File -FilePath "$stageDir\run.bat" -Encoding ascii

# 5. Create Portable ZIP Archive & Checksums
Write-Host "`n[5/6] Creating ZIP archive and cryptographic checksums..." -ForegroundColor Yellow
$zipPath = "$distDir\$bundleName.zip"
if (Test-Path $zipPath) { Remove-Item $zipPath -Force }
[System.IO.Compression.ZipFile]::CreateFromDirectory($stageDir, $zipPath, [System.IO.Compression.CompressionLevel]::Optimal, $false)
Write-Host "  Generated portable bundle: $zipPath ($(Get-Item $zipPath | Select-Object -ExpandProperty Length) bytes)" -ForegroundColor Green

# Compute SHA256 hashes
$shaExe = (Get-FileHash "$distDir\RTX_DLSS5_Studio.exe" -Algorithm SHA256).Hash
$shaZip = (Get-FileHash $zipPath -Algorithm SHA256).Hash

$checksums = @"
# RTX DLSS-5 Video Studio $Version - Official SHA-256 Checksums
# Generated on: $(Get-Date -Format "yyyy-MM-dd HH:mm:ss UTC")

$shaExe  RTX_DLSS5_Studio.exe
$shaZip  $bundleName.zip
"@

$checksums | Out-File -FilePath "$distDir\SHA256SUMS.txt" -Encoding ascii

# 6. Generate GitHub Release Notes Markdown
Write-Host "`n[6/6] Generating GitHub Release Notes..." -ForegroundColor Yellow

$releaseNotes = @"
# NVIDIA RTX DLSS-5 Video Studio $Version

Studio-grade video playback, super-resolution upscaling (VSR), TrueHDR conversion, and side-by-side comparison desktop application built on the **NVIDIA RTX Video SDK v1.1.0** and specifically optimized for **NVIDIA GeForce RTX 5090 (Blackwell Architecture)**.

---

### Highlights & Features
- **Standalone Portable Executable**: Zero-dependency direct download — automatically self-extracts and verifies NVIDIA neural runtime DLLs.
- **RTX 5090 Blackwell Hardware Optimization**: Deep 32GB GDDR7 lead-buffer ring caching 60-120 uncompressed 4K FP16 frames for stutter-free 144Hz+ playback.
- **Ultra VSR Quality 4 + TrueHDR**: Simultaneous neural 4K super-resolution and wide color gamut HDR conversion.
- **7 Comparison Modes**: Split-Screen Wipe, 50/50 Side-by-Side, Difference Heatmap, 2x2 Quad Grid, 8x Interactive Loupe Lens, and Baseline Bicubic.
- **Authenticode Cryptographic Validation**: Validates digital certificates of all runtime DLLs via `WinVerifyTrust`.
- **Offline High-Throughput Batch Upscaler**: Multi-threaded export pipeline with dual NVENC hardware encoders.

---

### Downloads & Assets

| Asset | Description |
| :--- | :--- |
| **`RTX_DLSS5_Studio.exe`** | Standalone 64-bit executable (Direct download, zero setup) |
| **`$bundleName.zip`** | Complete portable package with all runtime DLLs, HLSL shaders, and quick launcher |
| **`SHA256SUMS.txt`** | Official SHA-256 checksums for cryptographic authenticity |

---

### Cryptographic Checksums (SHA-256)
```text
$shaExe  RTX_DLSS5_Studio.exe
$shaZip  $bundleName.zip
```

---

### System Requirements
- **OS**: Windows 10 (21H2+) or Windows 11 64-bit
- **GPU**: NVIDIA GeForce RTX 20/30/40/50 Series (RTX 5090 Blackwell Recommended)
- **Driver**: NVIDIA Graphics Driver R550.50 or higher (R610+ Recommended)
"@

$releaseNotes | Out-File -FilePath "$distDir\release_notes.md" -Encoding utf8

Write-Host "`n=================================================================" -ForegroundColor Green
Write-Host " [SUCCESS] Release packaging completed successfully!" -ForegroundColor Green
Write-Host " Deliverables in: $distDir" -ForegroundColor Green
Write-Host "   1. $distDir\RTX_DLSS5_Studio.exe" -ForegroundColor Green
Write-Host "   2. $distDir\$bundleName.zip" -ForegroundColor Green
Write-Host "   3. $distDir\SHA256SUMS.txt" -ForegroundColor Green
Write-Host "   4. $distDir\release_notes.md" -ForegroundColor Green
Write-Host "=================================================================" -ForegroundColor Green

# 7. Optional GitHub Release Publication via gh CLI
if ($Publish) {
    Write-Host "`n[7/7] Publishing release to GitHub repository..." -ForegroundColor Yellow
    $ghArgs = @(
        "release", "create", "$Version",
        "$distDir\RTX_DLSS5_Studio.exe",
        "$zipPath",
        "$distDir\SHA256SUMS.txt",
        "$distDir\README.txt",
        "--title", "RTX DLSS-5 Video Studio $Version",
        "--notes-file", "$distDir\release_notes.md"
    )
    if ($Draft) { $ghArgs += "--draft" }
    if ($Prerelease) { $ghArgs += "--prerelease" }

    & gh @ghArgs
    if ($LASTEXITCODE -eq 0) {
        Write-Host "`n[SUCCESS] Release published to GitHub Releases section!" -ForegroundColor Green
    } else {
        Write-Warning "gh release create returned code $LASTEXITCODE"
    }
}
