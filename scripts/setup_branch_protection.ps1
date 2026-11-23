# Setup GitHub Branch Protection for 'main' branch
# Prerequisites: GitHub CLI (gh) authenticated via 'gh auth login'

$ErrorActionPreference = "Stop"

# Ensure gh executable is accessible
$gh = "gh"
if (-not (Get-Command gh -ErrorAction SilentlyContinue)) {
    $ghPath = "C:\Program Files\GitHub CLI\gh.exe"
    if (Test-Path $ghPath) {
        $gh = $ghPath
    } else {
        Write-Error "GitHub CLI (gh) is not installed or not in PATH."
        exit 1
    }
}

# Check auth status
Write-Host "Checking GitHub CLI authentication..." -ForegroundColor Cyan
& $gh auth status
if ($LASTEXITCODE -ne 0) {
    Write-Warning "You are not logged in. Please run: gh auth login"
    exit 1
}

$repoOwner = "theunrealfusion"
$repoName = "DLSS5-MediaStudio"
$branch = "main"
$protectionFile = Join-Path $PSScriptRoot "..\.github\branch-protection.json"

if (-not (Test-Path $protectionFile)) {
    Write-Error "Protection configuration file not found at: $protectionFile"
    exit 1
}

Write-Host "Applying branch protection to $repoOwner/$repoName ($branch)..." -ForegroundColor Cyan

& $gh api --method PUT `
    -H "Accept: application/vnd.github+json" `
    -H "X-GitHub-Api-Version: 2022-11-28" `
    "/repos/$repoOwner/$repoName/branches/$branch/protection" `
    --input $protectionFile

if ($LASTEXITCODE -eq 0) {
    Write-Host "Branch protection successfully applied to '$branch'!" -ForegroundColor Green
    Write-Host "Rules enforced:" -ForegroundColor Cyan
    Write-Host "  - PR required before merging"
    Write-Host "  - Required review from Code Owner (@kshvsharma960)"
    Write-Host "  - 1 approving review required"
    Write-Host "  - Dismiss stale reviews on new commits"
    Write-Host "  - Require last push approval"
    Write-Host "  - Require conversation resolution"
    Write-Host "  - Push/Merge restricted exclusively to @kshvsharma960"
    Write-Host "  - Direct push and force push blocked for everyone else"
} else {
    Write-Error "Failed to apply branch protection. Please check the error above."
}
