#----------------------------------------------------------------------------------
# File:        verify_all.ps1
# Project:     RTX DLSS-5 Video Studio
# Purpose:     Comprehensive edge-case and regression test suite
#----------------------------------------------------------------------------------

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectDir = Resolve-Path "$scriptDir\.."
$exe = "$projectDir\bin\x64\Release\RTX_DLSS5_Studio.exe"

Write-Host "=================================================================" -ForegroundColor Cyan
Write-Host " RTX DLSS-5 Video Studio: Comprehensive Test Suite" -ForegroundColor Cyan
Write-Host "=================================================================" -ForegroundColor Cyan

$testResults = @()

function Run-Check([string]$name, [scriptblock]$action) {
    Write-Host "`n[TEST] $name..." -ForegroundColor Yellow
    try {
        & $action
        Write-Host "[PASS] $name" -ForegroundColor Green
        $script:testResults += [PSCustomObject]@{ Test = $name; Status = "PASS" }
    } catch {
        Write-Host "[FAIL] $name : $_" -ForegroundColor Red
        $script:testResults += [PSCustomObject]@{ Test = $name; Status = "FAIL"; Error = "$_" }
    }
}

# 1. Hardware diagnostics
Run-Check "RTX 5090 Hardware Diagnostics" {
    $out = & $exe -diag | Out-String
    if ($out -notmatch "VSR Feature Creation: SUCCESS") {
        throw "Diagnostics failed: $out"
    }
}

# 2. MP4 Video input verification
Run-Check "MP4 Video Input (H.264 / 60 FPS)" {
    $sample = "$projectDir\data\test_sample.mp4"
    $out = & $exe -test "$sample" | Out-String
    if ($out -notmatch "ALL VERIFICATION CHECKS PASSED") {
        throw "MP4 verification failed: $out"
    }
}

# 3. Raw NV12 YUV video input verification
Run-Check "Raw NV12 Video Input (1280x720 / 30 FPS)" {
    $yuv = "$projectDir\data\NV12_1280_720_1280_30_synthetic_target.yuv"
    $out = & $exe -test "$yuv" | Out-String
    if ($out -notmatch "ALL VERIFICATION CHECKS PASSED") {
        throw "Raw NV12 verification failed: $out"
    }
}

# 4. Multi-container format testing (.mov, .m4v)
Run-Check "Multi-Container Format Support (MOV & M4V)" {
    $src = "$projectDir\data\test_sample.mp4"
    $mov = "$projectDir\data\test_temp.mov"
    $m4v = "$projectDir\data\test_temp.m4v"
    try {
        Copy-Item $src $mov
        Copy-Item $src $m4v
        $outMov = & $exe -test "$mov" | Out-String
        if ($outMov -notmatch "ALL VERIFICATION CHECKS PASSED") {
            throw "MOV container failed: $outMov"
        }
        $outM4v = & $exe -test "$m4v" | Out-String
        if ($outM4v -notmatch "ALL VERIFICATION CHECKS PASSED") {
            throw "M4V container failed: $outM4v"
        }
    } finally {
        Remove-Item $mov, $m4v -ErrorAction SilentlyContinue
    }
}

# 5. Path Sanitization & Quoting robustness
Run-Check "Path Sanitization & Wrapped Quotes" {
    $sample = "`"$projectDir\data\test_sample.mp4`""
    $out = & $exe -test $sample | Out-String
    if ($out -notmatch "ALL VERIFICATION CHECKS PASSED") {
        throw "Quoted path failed: $out"
    }
}

# 6. Non-existent file error handling (graceful failure without crash)
Run-Check "Graceful Error Handling for Missing File" {
    $missing = "$projectDir\data\non_existent_file_xyz.mp4"
    $proc = Start-Process -FilePath $exe -ArgumentList @("-test", "$missing") -Wait -PassThru -NoNewWindow
    if ($proc.ExitCode -eq 0) {
        throw "Expected non-zero exit code for missing file, but got 0"
    }
}

# Summary
Write-Host "`n=================================================================" -ForegroundColor Cyan
Write-Host " Test Execution Summary" -ForegroundColor Cyan
Write-Host "=================================================================" -ForegroundColor Cyan
$testResults | Format-Table -AutoSize

$failedCount = ($testResults | Where-Object { $_.Status -eq "FAIL" }).Count
if ($failedCount -gt 0) {
    Write-Host "`n[FAILED] $failedCount tests failed!" -ForegroundColor Red
    exit 1
} else {
    Write-Host "`n[SUCCESS] All $($testResults.Count) tests passed successfully!" -ForegroundColor Green
    exit 0
}
