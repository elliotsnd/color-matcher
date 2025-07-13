@echo off
REM Flash Reset Script for ESP32-S3
REM This script will completely erase flash and reflash the firmware

echo Starting flash reset process...

REM Step 1: Erase entire flash
echo Erasing entire flash...
esptool.py --chip esp32s3 --port COM6 erase_flash

if %ERRORLEVEL% neq 0 (
    echo ERROR: Flash erase failed!
    pause
    exit /b 1
)

echo Flash erased successfully!

REM Step 2: Build and upload firmware
echo Building and uploading firmware...
pio run --target upload

if %ERRORLEVEL% neq 0 (
    echo ERROR: Firmware upload failed!
    pause
    exit /b 1
)

echo Firmware uploaded successfully!

REM Step 3: Upload filesystem data
echo Uploading filesystem data...
python upload_data.py

if %ERRORLEVEL% neq 0 (
    echo WARNING: Filesystem upload failed - you may need to upload manually
)

echo Flash reset complete! You can now monitor the device.
echo To monitor: pio device monitor
pause
