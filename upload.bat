@echo off
echo ESP32 Color Sensor - Optimized Upload
echo =====================================
echo.

REM Check if Python is available
python --version >nul 2>&1
if errorlevel 1 (
    echo Error: Python not found. Please install Python first.
    pause
    exit /b 1
)

REM Run the optimized upload script
python upload_optimized.py

echo.
echo Upload process completed.
pause
