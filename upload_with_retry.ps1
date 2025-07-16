#!/usr/bin/env pwsh
# Auto-retry upload script for ESP32-S3 ProS3
# Retries upload up to 3 times if it fails

param(
    [string]$Port = "COM6",
    [int]$MaxRetries = 3,
    [int]$DelayBetweenRetries = 2
)

Write-Host "=== ESP32-S3 ProS3 Upload with Auto-Retry ===" -ForegroundColor Green
Write-Host "Port: $Port" -ForegroundColor Cyan
Write-Host "Max retries: $MaxRetries" -ForegroundColor Cyan
Write-Host "Delay between retries: $DelayBetweenRetries seconds" -ForegroundColor Cyan
Write-Host ""

$attempt = 1
$success = $false

while ($attempt -le $MaxRetries -and -not $success) {
    Write-Host "=== UPLOAD ATTEMPT $attempt of $MaxRetries ===" -ForegroundColor Yellow
    Write-Host "Running: pio run --target upload --upload-port $Port" -ForegroundColor Gray
    
    # Run PlatformIO upload command
    $process = Start-Process -FilePath "pio" -ArgumentList "run", "--target", "upload", "--upload-port", $Port -Wait -PassThru -NoNewWindow
    
    if ($process.ExitCode -eq 0) {
        Write-Host "✅ Upload successful on attempt $attempt!" -ForegroundColor Green
        $success = $true
    } else {
        Write-Host "❌ Upload failed on attempt $attempt (Exit code: $($process.ExitCode))" -ForegroundColor Red
        
        if ($attempt -lt $MaxRetries) {
            Write-Host "⏳ Waiting $DelayBetweenRetries seconds before retry..." -ForegroundColor Yellow
            Start-Sleep -Seconds $DelayBetweenRetries
            
            # Try to reset the device before next attempt
            Write-Host "🔄 Attempting device reset..." -ForegroundColor Cyan
            try {
                # Send reset command via esptool
                $resetProcess = Start-Process -FilePath "esptool.py" -ArgumentList "--port", $Port, "--chip", "esp32s3", "chip_id" -Wait -PassThru -NoNewWindow -ErrorAction SilentlyContinue
                if ($resetProcess.ExitCode -eq 0) {
                    Write-Host "🔄 Device reset successful" -ForegroundColor Green
                } else {
                    Write-Host "⚠️ Device reset failed, continuing anyway..." -ForegroundColor Yellow
                }
            } catch {
                Write-Host "⚠️ Could not reset device, continuing anyway..." -ForegroundColor Yellow
            }
        }
    }
    
    $attempt++
}

if ($success) {
    Write-Host ""
    Write-Host "🎉 UPLOAD COMPLETED SUCCESSFULLY!" -ForegroundColor Green
    Write-Host "Device should be running the updated firmware" -ForegroundColor Green
    
    # Optionally start serial monitor
    $monitor = Read-Host "Start serial monitor? (y/n)"
    if ($monitor -eq "y" -or $monitor -eq "Y") {
        Write-Host "Starting serial monitor..." -ForegroundColor Cyan
        pio device monitor --port $Port
    }
} else {
    Write-Host ""
    Write-Host "💥 UPLOAD FAILED AFTER $MaxRetries ATTEMPTS!" -ForegroundColor Red
    Write-Host "Troubleshooting tips:" -ForegroundColor Yellow
    Write-Host "1. Check if device is properly connected to $Port" -ForegroundColor Yellow
    Write-Host "2. Try pressing and holding BOOT button while uploading" -ForegroundColor Yellow
    Write-Host "3. Check if another application is using the serial port" -ForegroundColor Yellow
    Write-Host "4. Try a different USB cable or port" -ForegroundColor Yellow
    Write-Host "5. Reset the device manually and try again" -ForegroundColor Yellow
    
    exit 1
}
