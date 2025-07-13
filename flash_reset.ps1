# Flash Reset Script for ESP32-S3
# This script will completely erase flash and reflash the firmware

Write-Host "Starting flash reset process..." -ForegroundColor Green

# Step 1: Erase entire flash
Write-Host "Erasing entire flash..." -ForegroundColor Yellow
& esptool.py --chip esp32s3 --port COM6 erase_flash

if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Flash erase failed!" -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}

Write-Host "Flash erased successfully!" -ForegroundColor Green

# Step 2: Build and upload firmware
Write-Host "Building and uploading firmware..." -ForegroundColor Yellow
& pio run --target upload

if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Firmware upload failed!" -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}

Write-Host "Firmware uploaded successfully!" -ForegroundColor Green

# Step 3: Upload filesystem data
Write-Host "Uploading filesystem data..." -ForegroundColor Yellow
& python upload_data.py

if ($LASTEXITCODE -ne 0) {
    Write-Host "WARNING: Filesystem upload failed - you may need to upload manually" -ForegroundColor Yellow
}

Write-Host "Flash reset complete! You can now monitor the device." -ForegroundColor Green
Write-Host "To monitor: pio device monitor" -ForegroundColor Cyan
Read-Host "Press Enter to exit"
