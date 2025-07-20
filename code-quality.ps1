# Complete Code Quality Script for ESP32 Color-Matcher Project
# Runs both formatting and linting in the correct order

Write-Host "🚀 Starting complete code quality check for ESP32 Color-Matcher..." -ForegroundColor Green
Write-Host "=" * 60 -ForegroundColor Blue

# Step 1: Format code first
Write-Host "`n📝 STEP 1: Code Formatting" -ForegroundColor Magenta
& .\format-code.ps1

Write-Host "`n" + ("=" * 60) -ForegroundColor Blue

# Step 2: Run static analysis
Write-Host "`n🔍 STEP 2: Static Analysis & Linting" -ForegroundColor Magenta
& .\lint-code.ps1

Write-Host "`n" + ("=" * 60) -ForegroundColor Blue

# Step 3: Run PlatformIO check
Write-Host "`n⚡ STEP 3: PlatformIO Analysis" -ForegroundColor Magenta
Write-Host "Running integrated PlatformIO check..." -ForegroundColor Yellow
& pio check

Write-Host "`n" + ("=" * 60) -ForegroundColor Blue
Write-Host "🎉 Complete code quality check finished!" -ForegroundColor Green
Write-Host "💡 Your ESP32 color-matcher code is now formatted and analyzed" -ForegroundColor Blue
Write-Host "🔧 Review the output above for any remaining issues to address manually" -ForegroundColor Cyan
