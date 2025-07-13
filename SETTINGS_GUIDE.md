# 🎛️ Color Sensor Settings Guide

## Overview
All adjustable settings are now centralized in `src/sensor_settings.h` for easy tuning.

## Quick Start
1. Open `src/sensor_settings.h`
2. Find the setting you want to change
3. Modify the value after `#define`
4. Each setting shows its current default in comments

## Key Settings for Yellow Detection Issue

### Distance-Related Settings
```cpp
#define OPTIMAL_SENSOR_DISTANCE_MM 15         // Current: 15mm
#define SENSOR_SAMPLE_DELAY 2                 // Current: 2ms
#define COLOR_READING_SAMPLES 5               // Current: 3 samples
```

### Calibration Parameters
```cpp
#define CALIBRATION_IR_COMPENSATION 0.32f     // Current: 0.32
#define CALIBRATION_R_SLOPE 0.01352f          // Current: 0.01352
#define CALIBRATION_G_SLOPE 0.01535f          // Current: 0.01535  
#define CALIBRATION_B_SLOPE 0.02065f          // Current: 0.02065
```

### Yellow Detection Tuning
```cpp
#define YELLOW_DISTANCE_COMPENSATION true     // Enable distance tuning
#define YELLOW_MIN_RATIO 0.85                 // R+G vs B ratio
#define YELLOW_BRIGHTNESS_THRESHOLD 200       // Minimum brightness
```

## Settings Categories

### 🌐 Network Settings
- WiFi credentials (SSID, password)
- Static IP configuration  
- Access Point fallback
- Connection timeouts

### 🎨 Color Detection
- Sensor distance optimization
- Calibration parameters
- Yellow detection tuning
- RGB conversion settings

### 🧠 Performance
- KD-tree configuration
- Memory management
- Search timeouts
- PSRAM optimization

### 🐛 Debug & Logging
- Log level control
- Output formatting
- Performance monitoring
- Memory tracking

## Example: Fixing Yellow Detection

**Problem**: Yellow reads as RGB(209,214,68) close vs RGB(255,255,42) far

**Solution**: Adjust these settings in `sensor_settings.h`:
```cpp
// Increase sampling for better accuracy
#define COLOR_READING_SAMPLES 5               // Up from 3

// Enable yellow-specific tuning
#define YELLOW_DISTANCE_COMPENSATION true

// Fine-tune calibration if needed
#define CALIBRATION_G_SLOPE 0.01600f          // Slight increase from 0.01535
```

## Notes
- All defaults preserved in comments for reference
- Changes take effect after recompile and upload
- Settings are validated on startup
- Preset configurations available for common use cases

## Original Values Reference
Current defaults match the working values from `main.cpp`:
- WiFi: "Wifi 6" / "Scrofani1985"
- LED brightness: 80
- Integration time: 0x23
- Sample delay: 2ms
- IR compensation: 0.32
