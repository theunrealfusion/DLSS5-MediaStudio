#----------------------------------------------------------------------------------
# File:        run.ps1
# Project:     RTX DLSS-5 Video Studio
# Purpose:     Application launcher with preset configurations
#----------------------------------------------------------------------------------

param(
    [string]$InputFile = "",
    [int]$VSRQuality = 4,
    [switch]$TrueHDR,
    [switch]$FP16,
    [string]$Mode = "split",
    [switch]$Loupe,
    [string]$Batch = "",
    [switch]$Diag,
    [switch]$Test
)

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectDir = Resolve-Path "$scriptDir\.."
$exePath = "$projectDir\bin\x64\Release\RTX_DLSS5_Studio.exe"

if (-not (Test-Path $exePath)) {
    # Check debug fallback
    $exePath = "$projectDir\bin\x64\Debug\RTX_DLSS5_Studio.exe"
}

if (-not (Test-Path $exePath)) {
    Write-Host "[ERROR] Executable not found. Please run scripts\build.ps1 first." -ForegroundColor Red
    exit 1
}

$launchArgs = @()

if ($Test) {
    $launchArgs += "-test"
    if ($InputFile) {
        $launchArgs += "$InputFile"
    }
} elseif ($Diag) {
    $launchArgs += "-diag"
} elseif ($Batch) {
    if ($InputFile) {
        $launchArgs += @("-i", "$InputFile")
    }
    $launchArgs += @("-batch", "$Batch")
    $launchArgs += @("-vsrQuality", "$VSRQuality")
    if ($TrueHDR) { $launchArgs += "-thdr" }
} else {
    if ($InputFile) {
        $launchArgs += @("-i", "$InputFile")
    }
    $launchArgs += @("-vsr", "-vsrQuality", "$VSRQuality")
    if ($TrueHDR) {
        $launchArgs += "-thdr"
    }
    if ($FP16) {
        $launchArgs += "-fp16"
    }
    if ($Mode) {
        $launchArgs += @("-mode", "$Mode")
    }
    if ($Loupe) {
        $launchArgs += "-loupe"
    }
}

Write-Host "Launching RTX DLSS-5 Video Studio..." -ForegroundColor Cyan
Write-Host "Executable: $exePath" -ForegroundColor Gray
Write-Host "Arguments:  $($launchArgs -join ' ')" -ForegroundColor Gray

if ($Diag -or $Batch -or $Test) {
    Start-Process -FilePath $exePath -ArgumentList $launchArgs -Wait -NoNewWindow
} else {
    Start-Process -FilePath $exePath -ArgumentList $launchArgs
}
