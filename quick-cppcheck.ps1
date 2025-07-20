# Simple Cppcheck Analysis Script
param(
    [switch]$Verbose
)

Write-Host "🔍 Running Cppcheck analysis on your C++ code..." -ForegroundColor Green

# Check if Cppcheck is installed
$cppcheckPath = "C:\Program Files\Cppcheck\cppcheck.exe"
if (-not (Test-Path $cppcheckPath)) {
    Write-Host "❌ Cppcheck not found at $cppcheckPath!" -ForegroundColor Red
    Write-Host "Please ensure it's installed via: choco install cppcheck -y" -ForegroundColor Yellow
    exit 1
}

# Create reports directory
if (-not (Test-Path "cppcheck-reports")) {
    New-Item -ItemType Directory -Path "cppcheck-reports" -Force | Out-Null
}

Write-Host "🚀 Analyzing src/main.cpp..." -ForegroundColor Blue

# Run Cppcheck with basic settings for ESP32/Arduino
$cppcheckArgs = @(
    "src/main.cpp",
    "--enable=all",
    "--std=c++11", 
    "--platform=unix32",
    "--suppress=missingIncludeSystem",
    "--suppress=unusedFunction",
    "--force",
    "--xml",
    "--xml-version=2"
)

# Add ESP32 defines
$cppcheckArgs += @(
    "-DARDUINO=10819",
    "-DARDUINO_ESP32_DEV", 
    "-DESP32",
    "-DF_CPU=240000000L"
)

# Add include paths
$cppcheckArgs += @(
    "-Isrc",
    "-Ilib", 
    "-Iinclude"
)

$outputFile = "cppcheck-reports/cppcheck-report.xml"

try {
    # Run Cppcheck and capture stderr (where XML output goes)
    $process = Start-Process -FilePath $cppcheckPath -ArgumentList $cppcheckArgs -Wait -PassThru -RedirectStandardError $true -NoNewWindow
    $xmlOutput = $process.StandardError.ReadToEnd()
    
    # Save XML output
    $xmlOutput | Out-File -FilePath $outputFile -Encoding UTF8
    
    Write-Host "✅ Analysis complete!" -ForegroundColor Green
    Write-Host "📄 Report saved to: $outputFile" -ForegroundColor Blue
    
    # Try to parse and display results
    if ($xmlOutput.Trim()) {
        try {
            [xml]$xml = $xmlOutput
            $errors = $xml.results.errors.error
            
            if ($errors) {
                Write-Host ""
                Write-Host "⚠️ Found $($errors.Count) issues:" -ForegroundColor Yellow
                
                $errorCount = 0
                $warningCount = 0
                $otherCount = 0
                
                foreach ($item in $errors) {
                    $severity = $item.severity
                    $message = $item.msg
                    $file = if ($item.location) { Split-Path $item.location.file -Leaf } else { "unknown" }
                    $line = if ($item.location) { $item.location.line } else { "?" }
                    
                    switch ($severity) {
                        "error" { 
                            $errorCount++
                            Write-Host "  ERROR: $file line $line - $message" -ForegroundColor Red
                        }
                        "warning" { 
                            $warningCount++  
                            Write-Host "  WARNING: $file line $line - $message" -ForegroundColor Yellow
                        }
                        default { 
                            $otherCount++
                            if ($Verbose) {
                                Write-Host "  $severity`: $file line $line - $message" -ForegroundColor Blue
                            }
                        }
                    }
                }
                
                Write-Host ""
                Write-Host "📊 Summary:" -ForegroundColor Cyan
                Write-Host "  🔴 Errors: $errorCount" -ForegroundColor Red
                Write-Host "  🟡 Warnings: $warningCount" -ForegroundColor Yellow  
                Write-Host "  🔵 Other: $otherCount" -ForegroundColor Blue
                
            } else {
                Write-Host "✅ No issues found! Clean code! 🎉" -ForegroundColor Green
            }
        } catch {
            Write-Host "Raw Cppcheck output:" -ForegroundColor Yellow
            Write-Host $xmlOutput
        }
    } else {
        Write-Host "✅ No issues found! Clean code! 🎉" -ForegroundColor Green
    }
    
} catch {
    Write-Host "❌ Error running Cppcheck: $_" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "💡 Next steps:" -ForegroundColor Cyan  
Write-Host "  • Fix any 🔴 errors first (likely real bugs)" -ForegroundColor White
Write-Host "  • Review 🟡 warnings (potential issues)" -ForegroundColor White
Write-Host "  • Run with -Verbose to see all issues" -ForegroundColor White
Write-Host "  • Check the detailed XML report for more info" -ForegroundColor White
