@echo off
REM Color Matcher Build and Upload Script for Windows
REM This script compiles and uploads the project with real-time settings support

echo 🚀 Building Color Matcher with Real-Time Settings...
echo ==========================================

REM Check if platformio is installed
where pio >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo ❌ PlatformIO CLI not found. Please install it first.
    pause
    exit /b 1
)

REM Clean previous build
echo 🧹 Cleaning previous build...
pio run --target clean

REM Build the project
echo 🔨 Building project...
pio run
if %ERRORLEVEL% NEQ 0 (
    echo ❌ Build failed. Please check the errors above.
    pause
    exit /b 1
)

echo ✅ Build successful!

REM Ask user if they want to upload
set /p upload=📤 Upload to device? (y/N): 
if /i "%upload%"=="y" (
    echo 📤 Uploading to ESP32...
    pio run --target upload
    if %ERRORLEVEL% EQU 0 (
        echo ✅ Upload successful!
        echo.
        echo 🎉 Real-time settings are now enabled!
        echo 📱 Open the web interface to adjust settings live
        echo 🌐 Default IP: 192.168.0.152
        echo.
        echo 🔧 Features added:
        echo    • LED brightness slider (real-time)
        echo    • Integration time selector (real-time)
        echo    • IR compensation sliders (real-time)
        echo    • Color sampling controls
        echo    • Debug toggles
        echo    • Settings persistence
    ) else (
        echo ❌ Upload failed.
        pause
        exit /b 1
    )
) else (
    echo 🏗️ Build completed. Upload skipped.
)

echo.
echo 📚 Usage:
echo    1. Connect to the device's web interface
echo    2. Scroll down to 'Real-Time Settings' panel
echo    3. Adjust sliders and dropdowns for immediate effect
echo    4. Use 'Load Current' to refresh from device
echo    5. Use 'Apply Settings' to save all changes
echo    6. Use 'Reset to Defaults' to restore factory settings

pause
