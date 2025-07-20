@echo off
echo 🔍 Running Cppcheck analysis on Color Sensor Project...

REM Check if Cppcheck is installed
where cppcheck >nul 2>&1
if %errorlevel% neq 0 (
    echo ❌ Cppcheck not found! Please install it first.
    echo Download from: https://cppcheck.sourceforge.io/
    echo Or install via: winget install Cppcheck.Cppcheck
    pause
    exit /b 1
)

REM Create reports directory
if not exist "cppcheck-reports" mkdir "cppcheck-reports"

echo 🚀 Analyzing src/main.cpp...

REM Run Cppcheck with ESP32/Arduino settings
cppcheck src/main.cpp ^
  --enable=all ^
  --std=c++11 ^
  --platform=unix32 ^
  --suppress=missingIncludeSystem ^
  --suppress=unusedFunction ^
  --force ^
  --xml ^
  --xml-version=2 ^
  -DARDUINO=10819 ^
  -DARDUINO_ESP32_DEV ^
  -DESP32 ^
  -DF_CPU=240000000L ^
  -Isrc ^
  -Ilib ^
  -Iinclude ^
  2> "cppcheck-reports/cppcheck-report.xml"

if %errorlevel% equ 0 (
    echo ✅ Analysis complete!
    echo 📄 Report saved to: cppcheck-reports/cppcheck-report.xml
    
    REM Count issues in the XML file
    findstr /c:"severity=" "cppcheck-reports/cppcheck-report.xml" >nul 2>&1
    if %errorlevel% equ 0 (
        echo ⚠️ Issues found - check the XML report for details
    ) else (
        echo ✅ No issues found! Clean code! 🎉
    )
) else (
    echo ❌ Analysis failed!
)

echo.
echo 💡 Next steps:
echo   • Open cppcheck-reports/cppcheck-report.xml to view detailed results
echo   • Fix any errors first (likely real bugs)
echo   • Review warnings (potential issues)
echo   • Consider style suggestions for better code quality

pause
