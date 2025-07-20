# Cppcheck Analysis Script for Color Sensor Project
# Fast static analysis for C++ code quality

param(
    [switch]$Verbose,
    [switch]$OpenReport,
    [string]$OutputFormat = "xml"
)

Write-Host "🔍 Running Cppcheck analysis on Color Sensor Project..." -ForegroundColor Green

# Use the installed Cppcheck from Chocolatey
$cppcheckPath = "C:\Program Files\Cppcheck\cppcheck.exe"
if (-not (Test-Path $cppcheckPath)) {
    Write-Host "❌ Cppcheck not found at $cppcheckPath!" -ForegroundColor Red
    Write-Host "Please ensure Cppcheck is installed via: choco install cppcheck -y" -ForegroundColor Yellow
    Write-Host "Or use: winget install Cppcheck.Cppcheck" -ForegroundColor Yellow
    exit 1
}

# Create output directory
$outputDir = "cppcheck-reports"
if (-not (Test-Path $outputDir)) {
    New-Item -ItemType Directory -Path $outputDir -Force | Out-Null
}

# Get PlatformIO include paths
$includeArgs = @()
if (Test-Path ".pio\build") {
    $buildDirs = Get-ChildItem ".pio\build" -Directory
    if ($buildDirs) {
        $buildDir = $buildDirs[0].FullName
        Write-Host "📁 Using build directory: $buildDir" -ForegroundColor Blue
        
        # Add common ESP32/Arduino include paths
        $includeArgs += @(
            "--includes-file=.pio\build\*\*",
            "-I", "$env:USERPROFILE\.platformio\packages\framework-arduinoespressif32\cores\esp32",
            "-I", "$env:USERPROFILE\.platformio\packages\framework-arduinoespressif32\libraries",
            "-I", "lib",
            "-I", "src",
            "-I", "include"
        )
    }
}

# Define analysis parameters
$sourceFiles = @("src\main.cpp")
if (Test-Path "src") {
    $additionalSources = Get-ChildItem "src" -Filter "*.cpp" -Recurse | Where-Object { $_.Name -ne "main.cpp" }
    $sourceFiles += $additionalSources | ForEach-Object { $_.FullName }
}

# Add library sources if they exist
if (Test-Path "lib") {
    $libSources = Get-ChildItem "lib" -Filter "*.cpp" -Recurse
    $sourceFiles += $libSources | ForEach-Object { $_.FullName }
}

Write-Host "📝 Analyzing files:" -ForegroundColor Blue
$sourceFiles | ForEach-Object { Write-Host "   • $_" -ForegroundColor Gray }

# Cppcheck arguments for embedded C++ analysis
$cppcheckArgs = @(
    "--enable=all",                    # Enable all checks
    "--std=c++11",                     # C++11 standard (Arduino compatible)
    "--platform=unix32",               # 32-bit platform like ESP32
    "--suppress=missingIncludeSystem", # Suppress system include warnings
    "--suppress=unusedFunction",       # Arduino functions might appear unused
    "--suppress=unmatchedSuppression", # Don't warn about unused suppressions
    "--force",                         # Force checking even with missing includes
    "--inline-suppr",                  # Allow inline suppressions
    "--quiet"                          # Reduce noise
)

# Add includes if found
$cppcheckArgs += $includeArgs

# ESP32/Arduino specific defines
$cppcheckArgs += @(
    "-DARDUINO=10819",
    "-DARDUINO_ESP32_DEV",
    "-DESP32",
    "-DESP_PLATFORM",
    "-DF_CPU=240000000L",
    "-DHAVE_CONFIG_H"
)

# Output format specific arguments
if ($OutputFormat -eq "xml") {
    $cppcheckArgs += @("--xml", "--xml-version=2")
    $outputFile = "$outputDir\cppcheck-report.xml"
} elseif ($OutputFormat -eq "json") {
    $cppcheckArgs += "--output-file=$outputDir\cppcheck-report.json"
    $outputFile = "$outputDir\cppcheck-report.json"
} else {
    $outputFile = "$outputDir\cppcheck-report.txt"
}

# Add source files
$cppcheckArgs += $sourceFiles

Write-Host "🚀 Running Cppcheck analysis..." -ForegroundColor Blue
if ($Verbose) {
    Write-Host "Command: cppcheck $($cppcheckArgs -join ' ')" -ForegroundColor Gray
}

# Run Cppcheck
$stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
$process = Start-Process -FilePath $cppcheckPath -ArgumentList $cppcheckArgs -RedirectStandardError $true -RedirectStandardOutput $true -PassThru -Wait

$stdout = $process.StandardOutput.ReadToEnd()
$stderr = $process.StandardError.ReadToEnd()
$stopwatch.Stop()

# Save output
if ($OutputFormat -eq "xml") {
    $stderr | Out-File -FilePath $outputFile -Encoding UTF8
} else {
    $stdout | Out-File -FilePath $outputFile -Encoding UTF8
    if ($stderr) {
        $stderr | Out-File -FilePath $outputFile -Append -Encoding UTF8
    }
}

Write-Host "⏱️  Analysis completed in $($stopwatch.Elapsed.TotalSeconds.ToString('F1')) seconds" -ForegroundColor Green

# Parse and display results
if (Test-Path $outputFile) {
    Write-Host "📊 Analysis Results:" -ForegroundColor Cyan
    
    if ($OutputFormat -eq "xml") {
        try {
            [xml]$xmlReport = Get-Content $outputFile -Raw
            $errors = $xmlReport.results.errors.error
            
            if ($errors) {
                Write-Host "⚠️  Found $($errors.Count) issues:" -ForegroundColor Yellow
                
                $errorCount = 0
                $warningCount = 0
                $infoCount = 0
                
                foreach ($error in $errors) {
                    $severity = $error.severity
                    $message = $error.msg
                    $file = Split-Path $error.location.file -Leaf
                    $line = $error.location.line
                    
                    switch ($severity) {
                        "error" { 
                            $errorCount++
                            Write-Host "  🔴 ERROR: $file:$line - $message" -ForegroundColor Red
                        }
                        "warning" { 
                            $warningCount++
                            Write-Host "  🟡 WARNING: $file:$line - $message" -ForegroundColor Yellow
                        }
                        default { 
                            $infoCount++
                            if ($Verbose) {
                                Write-Host "  🔵 $severity`: $file:$line - $message" -ForegroundColor Blue
                            }
                        }
                    }
                }
                
                Write-Host ""
                Write-Host "📈 Summary:" -ForegroundColor Cyan
                Write-Host "  🔴 Errors: $errorCount" -ForegroundColor Red
                Write-Host "  🟡 Warnings: $warningCount" -ForegroundColor Yellow
                Write-Host "  🔵 Info/Style: $infoCount" -ForegroundColor Blue
                
            } else {
                Write-Host "✅ No issues found! Clean code! 🎉" -ForegroundColor Green
            }
        } catch {
            Write-Host "📝 Raw output saved to: $outputFile" -ForegroundColor Blue
            if ($stderr) {
                Write-Host $stderr -ForegroundColor Yellow
            }
        }
    } else {
        # For non-XML output, just show the content
        $content = Get-Content $outputFile -Raw
        if ($content.Trim()) {
            Write-Host $content -ForegroundColor Yellow
        } else {
            Write-Host "✅ No issues found! Clean code! 🎉" -ForegroundColor Green
        }
    }
    
    Write-Host ""
    Write-Host "📄 Full report saved to: $outputFile" -ForegroundColor Blue
    
    # Generate HTML report if possible
    if ($OutputFormat -eq "xml" -and (Get-Command "cppcheck-htmlreport" -ErrorAction SilentlyContinue)) {
        Write-Host "🌐 Generating HTML report..." -ForegroundColor Blue
        $htmlOutput = "$outputDir\cppcheck-report.html"
        & cppcheck-htmlreport --file=$outputFile --report-dir=$outputDir --source-dir=src
        
        if (Test-Path "$outputDir\index.html") {
            Write-Host "✅ HTML report generated: $outputDir\index.html" -ForegroundColor Green
            
            if ($OpenReport) {
                Write-Host "🌐 Opening report in browser..." -ForegroundColor Blue
                Start-Process "$outputDir\index.html"
            }
        }
    }
    
} else {
    Write-Host "❌ Analysis failed! No output file generated." -ForegroundColor Red
    if ($stderr) {
        Write-Host "Error output:" -ForegroundColor Red
        Write-Host $stderr -ForegroundColor Yellow
    }
    exit 1
}

# Quick fix suggestions
Write-Host ""
Write-Host "💡 Quick Fix Suggestions:" -ForegroundColor Cyan
Write-Host "  1. Fix any 🔴 errors first - these are likely real bugs" -ForegroundColor White
Write-Host "  2. Review 🟡 warnings - these might indicate potential issues" -ForegroundColor White
Write-Host "  3. Consider style suggestions for better code quality" -ForegroundColor White
Write-Host "  4. Add // cppcheck-suppress [rule] comments for false positives" -ForegroundColor White

Write-Host ""
Write-Host "🎯 To run with different options:" -ForegroundColor Cyan
Write-Host "  • Verbose output: .\cppcheck-analysis.ps1 -Verbose" -ForegroundColor White
Write-Host "  • Open HTML report: .\cppcheck-analysis.ps1 -OpenReport" -ForegroundColor White
Write-Host "  • JSON output: .\cppcheck-analysis.ps1 -OutputFormat json" -ForegroundColor White

Write-Host ""
Write-Host "✨ Analysis complete!" -ForegroundColor Green
