# Clang-Format Script for ESP32 Color-Matcher Project
# Formats all C/C++ source files using the project's .clang-format configuration

Write-Host "🎨 Formatting C++ code for ESP32 Color-Matcher project..." -ForegroundColor Green

# Format main source files
Write-Host "`n📁 Formatting src/ directory..." -ForegroundColor Yellow
Get-ChildItem -Path "src" -Include "*.cpp", "*.h", "*.hpp" -Recurse | ForEach-Object {
    Write-Host "  Formatting: $($_.Name)" -ForegroundColor Cyan
    clang-format -i $_.FullName
}

# Format library files if they exist
if (Test-Path "lib") {
    Write-Host "`n📁 Formatting lib/ directory..." -ForegroundColor Yellow
    Get-ChildItem -Path "lib" -Include "*.cpp", "*.h", "*.hpp" -Recurse | ForEach-Object {
        Write-Host "  Formatting: $($_.Name)" -ForegroundColor Cyan
        clang-format -i $_.FullName
    }
}

# Format include files if they exist
if (Test-Path "include") {
    Write-Host "`n📁 Formatting include/ directory..." -ForegroundColor Yellow
    Get-ChildItem -Path "include" -Include "*.h", "*.hpp" -Recurse | ForEach-Object {
        Write-Host "  Formatting: $($_.Name)" -ForegroundColor Cyan
        clang-format -i $_.FullName
    }
}

Write-Host "`n✅ Code formatting complete!" -ForegroundColor Green
Write-Host "💡 All files have been formatted according to your .clang-format configuration" -ForegroundColor Blue
