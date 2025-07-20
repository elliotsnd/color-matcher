# Clang-Tidy Script for ESP32 Color-Matcher Project
# Analyzes and fixes C++ code issues using the project's .clang-tidy configuration

Write-Host "🔍 Running static analysis for ESP32 Color-Matcher project..." -ForegroundColor Green

# Check if compile_commands.json exists for better analysis
if (Test-Path "compile_commands.json") {
    Write-Host "✅ Found compile_commands.json - using for accurate analysis" -ForegroundColor Blue
    $useCompileDb = "-p ."
} else {
    Write-Host "⚠️  No compile_commands.json found - generating with PlatformIO..." -ForegroundColor Yellow
    Write-Host "   Running: pio run -t compiledb" -ForegroundColor Cyan
    & pio run -t compiledb
    if (Test-Path "compile_commands.json") {
        $useCompileDb = "-p ."
        Write-Host "✅ Generated compile_commands.json successfully" -ForegroundColor Green
    } else {
        $useCompileDb = ""
        Write-Host "⚠️  Could not generate compile database, proceeding without it" -ForegroundColor Yellow
    }
}

# Analyze main source files
Write-Host "`n📁 Analyzing src/ directory..." -ForegroundColor Yellow
Get-ChildItem -Path "src" -Include "*.cpp", "*.c" -Recurse | ForEach-Object {
    Write-Host "  Analyzing: $($_.Name)" -ForegroundColor Cyan
    if ($useCompileDb) {
        & clang-tidy $_.FullName $useCompileDb --fix --format-style=file
    } else {
        & clang-tidy $_.FullName --fix --format-style=file
    }
}

# Analyze library files if they exist
if (Test-Path "lib") {
    Write-Host "`n📁 Analyzing lib/ directory..." -ForegroundColor Yellow
    Get-ChildItem -Path "lib" -Include "*.cpp", "*.c" -Recurse | ForEach-Object {
        Write-Host "  Analyzing: $($_.Name)" -ForegroundColor Cyan
        if ($useCompileDb) {
            & clang-tidy $_.FullName $useCompileDb --fix --format-style=file
        } else {
            & clang-tidy $_.FullName --fix --format-style=file
        }
    }
}

Write-Host "`n✅ Static analysis complete!" -ForegroundColor Green
Write-Host "💡 All fixable issues have been resolved according to your .clang-tidy configuration" -ForegroundColor Blue
Write-Host "🔧 Review any remaining warnings that require manual attention" -ForegroundColor Magenta
