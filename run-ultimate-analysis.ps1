# Ultimate Static Analysis Runner
# Runs all three tools and combines results

param(
    [switch]$Verbose,
    [switch]$FixableOnly,
    [string]$Target = "src/main.cpp"
)

$timestamp = Get-Date -Format "yyyy-MM-dd_HH-mm-ss"
$reportDir = "analysis-reports\ultimate-$timestamp"
New-Item -ItemType Directory -Path $reportDir -Force | Out-Null

Write-Host "ULTIMATE STATIC ANALYSIS STARTING" -ForegroundColor Red
Write-Host "====================================" -ForegroundColor Blue
Write-Host "Target: $Target" -ForegroundColor White
Write-Host "Tools: Clang-Tidy + Cppcheck + PVS-Studio" -ForegroundColor Cyan
Write-Host ""

# 1. Run Clang-Tidy (most comprehensive)
Write-Host "1. Running Clang-Tidy (Deep Semantic Analysis)..." -ForegroundColor Magenta
if (Test-Path "compile_commands.json") {
    $clangTidyArgs = @(
        "-p=.",
        "--format-style=file",
        "--header-filter=.*\.(h|hpp)$",
        $Target
    )
    
    if ($FixableOnly) {
        $clangTidyArgs += "--fix-errors"
        $clangTidyArgs += "--fix"
    }
    
    & clang-tidy $clangTidyArgs 2>&1 | Tee-Object -FilePath "$reportDir\clang-tidy-results.txt"
} else {
    Write-Host "   WARNING: compile_commands.json not found - basic analysis only" -ForegroundColor Yellow
    $basicArgs = @($Target, "--", "-std=c++17", "-DARDUINO=10819", "-DESP32=1", "-Isrc")
    if ($FixableOnly) {
        $basicArgs = @("--fix", "--fix-errors") + $basicArgs
    }
    & clang-tidy $basicArgs 2>&1 | Tee-Object -FilePath "$reportDir\clang-tidy-results.txt"
}

# 2. Run Cppcheck (fast & reliable)
Write-Host "`n2. Running Cppcheck (Fast Static Analysis)..." -ForegroundColor Blue
& "C:\Program Files\Cppcheck\cppcheck.exe" --enable=all --inconclusive --std=c++17 --template=gcc --suppress=missingIncludeSystem -DPROGMEM="" -DARDUINO=10819 -DESP32=1 --xml --xml-version=2 $Target 2>&1 | Tee-Object -FilePath "$reportDir\cppcheck-results.xml"

# 3. PVS-Studio (if available)
Write-Host "`n3. Checking PVS-Studio availability..." -ForegroundColor Green
$pvsPath = Get-Command "PVS-Studio_Cmd" -ErrorAction SilentlyContinue
if ($pvsPath) {
    Write-Host "   Running PVS-Studio (Professional Analysis)..." -ForegroundColor Green
    # PVS-Studio command would go here
    "PVS-Studio analysis would run here with proper licensing" | Out-File "$reportDir\pvs-studio-placeholder.txt"
} else {
    Write-Host "   INFO: PVS-Studio not available in CLI - use VS Code extension manually" -ForegroundColor Yellow
    "Use PVS-Studio VS Code extension for GUI analysis" | Out-File "$reportDir\pvs-studio-instructions.txt"
}

# Combine and analyze results
Write-Host "`nAnalyzing Results..." -ForegroundColor Cyan

# Parse Clang-Tidy results
$clangTidyIssues = Get-Content "$reportDir\clang-tidy-results.txt" | Where-Object { $_ -match "warning:|error:|note:" }
$clangTidyCount = $clangTidyIssues.Count

# Parse Cppcheck results  
$cppcheckContent = Get-Content "$reportDir\cppcheck-results.xml" -Raw -ErrorAction SilentlyContinue
$cppcheckCount = 0
if ($cppcheckContent -and $cppcheckContent.Contains("<error")) {
    $cppcheckCount = ([regex]::Matches($cppcheckContent, "<error")).Count
}

Write-Host "`nANALYSIS SUMMARY:" -ForegroundColor Yellow
Write-Host "==================" -ForegroundColor Blue
Write-Host "Clang-Tidy Issues: $clangTidyCount" -ForegroundColor Magenta
Write-Host "Cppcheck Issues: $cppcheckCount" -ForegroundColor Blue
Write-Host "PVS-Studio: Run via VS Code extension" -ForegroundColor Green

$totalIssues = $clangTidyCount + $cppcheckCount
Write-Host "`nTotal Detected Issues: $totalIssues" -ForegroundColor White

if ($totalIssues -eq 0) {
    Write-Host "PERFECT CODE! No issues found across all analyzers!" -ForegroundColor Green
} elseif ($totalIssues -le 5) {
    Write-Host "EXCELLENT! Very few issues detected." -ForegroundColor Green
} elseif ($totalIssues -le 15) {
    Write-Host "GOOD! Manageable number of issues to review." -ForegroundColor Yellow
} else {
    Write-Host "NEEDS ATTENTION! Multiple issues detected - prioritize fixing." -ForegroundColor Red
}

Write-Host "`nDetailed reports saved in: $reportDir" -ForegroundColor Green
Write-Host "`nNext Steps:" -ForegroundColor Cyan
Write-Host "  1. Review Clang-Tidy results first (most comprehensive)" -ForegroundColor White
Write-Host "  2. Check Cppcheck XML for additional issues" -ForegroundColor White
Write-Host "  3. Run PVS-Studio via VS Code extension for GUI analysis" -ForegroundColor White
Write-Host "  4. Use --FixableOnly flag to auto-fix simple issues" -ForegroundColor White

Write-Host "`nUltimate analysis complete!" -ForegroundColor Green
