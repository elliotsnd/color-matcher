@echo off
REM Auto-retry upload script for ESP32-S3 ProS3
REM Retries upload up to 3 times if it fails

setlocal enabledelayedexpansion

set PORT=COM6
set MAX_RETRIES=3
set DELAY=2
set ATTEMPT=1
set SUCCESS=0

echo === ESP32-S3 ProS3 Upload with Auto-Retry ===
echo Port: %PORT%
echo Max retries: %MAX_RETRIES%
echo Delay between retries: %DELAY% seconds
echo.

:retry_loop
echo === UPLOAD ATTEMPT %ATTEMPT% of %MAX_RETRIES% ===
echo Running: pio run --target upload --upload-port %PORT%

REM Run PlatformIO upload command
pio run --target upload --upload-port %PORT%

if %errorlevel% equ 0 (
    echo ✅ Upload successful on attempt %ATTEMPT%!
    set SUCCESS=1
    goto :success
) else (
    echo ❌ Upload failed on attempt %ATTEMPT% ^(Exit code: %errorlevel%^)
    
    if %ATTEMPT% lss %MAX_RETRIES% (
        echo ⏳ Waiting %DELAY% seconds before retry...
        timeout /t %DELAY% /nobreak > nul
        
        echo 🔄 Attempting device reset...
        REM Try to reset the device before next attempt
        esptool.py --port %PORT% --chip esp32s3 chip_id >nul 2>&1
        if %errorlevel% equ 0 (
            echo 🔄 Device reset successful
        ) else (
            echo ⚠️ Device reset failed, continuing anyway...
        )
    )
)

set /a ATTEMPT=%ATTEMPT%+1
if %ATTEMPT% leq %MAX_RETRIES% goto :retry_loop

:failed
echo.
echo 💥 UPLOAD FAILED AFTER %MAX_RETRIES% ATTEMPTS!
echo Troubleshooting tips:
echo 1. Check if device is properly connected to %PORT%
echo 2. Try pressing and holding BOOT button while uploading
echo 3. Check if another application is using the serial port
echo 4. Try a different USB cable or port
echo 5. Reset the device manually and try again
exit /b 1

:success
echo.
echo 🎉 UPLOAD COMPLETED SUCCESSFULLY!
echo Device should be running the updated firmware
echo.
set /p MONITOR="Start serial monitor? (y/n): "
if /i "%MONITOR%"=="y" (
    echo Starting serial monitor...
    pio device monitor --port %PORT%
)
