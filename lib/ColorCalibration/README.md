# 5-Point Color Correction Matrix (CCM) Calibration System

A complete, mathematically robust color calibration system for ESP32-S3 with TCS3430 color sensor, replacing flawed local-patch calibration with global transformation matrix approach.

## Overview

This system implements a 5-point color calibration using least-squares approximation to calculate an optimal 3x3 Color Correction Matrix (CCM). The matrix transforms raw XYZ sensor readings into accurate RGB values using the formula:

```
RGB_corrected = CCM * XYZ_normalized
```

Where CCM is calculated using:
```
coefficients = (A^T * A)^-1 * A^T * b
```

## Features

- **Mathematically Robust**: Uses least-squares approximation for optimal matrix calculation
- **5-Point Calibration**: Black, White, Grey, Blue, Yellow reference colors
- **Persistent Storage**: Automatic saving/loading via ESP32 Preferences
- **Web API**: Complete REST endpoints for web-based calibration
- **Drop-in Replacement**: Backward compatible with legacy functions
- **Real-time Validation**: Matrix stability and quality assessment
- **Error Handling**: Comprehensive validation and graceful fallbacks

## File Structure

```
lib/ColorCalibration/
├── CalibrationStructures.h    # Core data structures
├── MatrixSolver.h/cpp         # Least-squares matrix solver
├── ColorCalibrationManager.h/cpp # Main calibration manager
├── CalibrationEndpoints.h/cpp # REST API endpoints
├── ColorCalibration.h/cpp     # Main interface
└── README.md                  # This documentation
```

## Quick Start

### 1. Basic Usage

```cpp
#include "lib/ColorCalibration/ColorCalibration.h"

void setup() {
    Serial.begin(115200);
    
    // Initialize calibration system
    if (!ColorCalibration::initialize()) {
        Serial.println("Failed to initialize calibration");
    }
}

void loop() {
    // Read sensor values
    uint16_t rawX = readSensorX();
    uint16_t rawY = readSensorY();
    uint16_t rawZ = readSensorZ();
    
    // Convert to RGB with calibration
    uint8_t r, g, b;
    if (ColorCalibration::convertColor(rawX, rawY, rawZ, r, g, b)) {
        Serial.printf("Calibrated RGB: (%d, %d, %d)\n", r, g, b);
    } else {
        Serial.println("Using fallback conversion");
    }
}
```

### 2. Drop-in Replacement

Replace legacy color conversion with:

```cpp
// Old way
// convertXYZtoRGB(rawX, rawY, rawZ, r, g, b);

// New way (drop-in replacement)
COLOR_CALIBRATION_INIT();
COLOR_CALIBRATION_CONVERT(rawX, rawY, rawZ, r, g, b);
```

### 3. Web-based Calibration

```cpp
#include <ESPAsyncWebServer.h>
#include "lib/ColorCalibration/CalibrationEndpoints.h"

AsyncWebServer server(80);
CalibrationEndpoints endpoints(server);

void setup() {
    // Initialize calibration system
    ColorCalibration::initialize();
    
    // Initialize web endpoints
    endpoints.initialize();
    
    // Start web server
    server.begin();
}
```

## API Reference

### REST Endpoints

#### Calibration Endpoints
- `POST /api/calibrate-black?x=123&y=456&z=789`
- `POST /api/calibrate-white?x=123&y=456&z=789`
- `POST /api/calibrate-grey?x=123&y=456&z=789`
- `POST /api/calibrate-blue?x=123&y=456&z=789`
- `POST /api/calibrate-yellow?x=123&y=456&z=789`

#### Management Endpoints
- `GET /api/calibration-status` - Get current status
- `POST /api/reset-calibration` - Reset all calibration data
- `GET /api/calibration-debug` - Get debug information (debug mode only)

### Response Format

All endpoints return JSON responses:

**Success:**
```json
{
    "success": true,
    "color": "white"
}
```

**Error:**
```json
{
    "error": "Invalid sensor reading"
}
```

**Status:**
```json
{
    "black_calibrated": true,
    "white_calibrated": true,
    "grey_calibrated": true,
    "blue_calibrated": true,
    "yellow_calibrated": true,
    "progress": 100,
    "is_complete": true,
    "ccm_valid": true,
    "ccm_determinant": 0.123,
    "ccm_condition_number": 45.6
}
```

## Target Colors

The system uses these exact reference colors:

| Color | RGB Value | Name |
|-------|-----------|------|
| Black | (5, 5, 5) | Near-black reference |
| White | (247, 248, 244) | Vivid White |
| Grey | (136, 138, 137) | Timeless Grey |
| Blue | (40, 60, 180) | Deep Blue |
| Yellow | (230, 220, 50) | Bright Yellow |

## Mathematical Details

### Matrix Calculation

The system solves the least-squares problem:

```
min ||A * x - b||²
```

Where:
- A = [X Y Z] matrix of normalized sensor readings
- x = [r₁ r₂ r₃]ᵀ coefficients for each RGB channel
- b = target RGB values (normalized 0-1)

### Matrix Validation

The system validates the CCM using:
- **Determinant**: Must be > 1e-6 for stability
- **Condition Number**: Must be < 1000 for numerical stability

### Normalization

Raw XYZ values are normalized to 0-1 range:
```
normalized = raw / 65535
```

## Integration Guide

### 1. PlatformIO Configuration

Add to `platformio.ini`:
```ini
lib_deps = 
    ESP Async WebServer
```

### 2. Include in Project

```cpp
#include "lib/ColorCalibration/ColorCalibration.h"
```

### 3. Initialize System

```cpp
void setup() {
    Serial.begin(115200);
    
    // Initialize calibration system
    if (!ColorCalibration::initialize()) {
        Serial.println("Calibration init failed");
    }
}
```

### 4. Use in Code

```cpp
// Get sensor readings
uint16_t x = tcs3430.readX();
uint16_t y = tcs3430.readY();
uint16_t z = tcs3430.readZ();

// Convert with calibration
uint8_t r, g, b;
ColorCalibration::convertColor(x, y, z, r, g, b);
```

## Advanced Usage

### Manual Calibration Points

```cpp
// Add calibration point programmatically
ColorCalibration::getManager().addOrUpdateCalibrationPoint(
    "white", 45000, 46000, 44000, 0.95f
);
```

### Check Calibration Status

```cpp
CalibrationStatus status = ColorCalibration::getManager().getCalibrationStatus();
Serial.printf("Calibration progress: %d%%\n", status.getProgress());
```

### Get Matrix for Debugging

```cpp
ColorCorrectionMatrix ccm = ColorCalibration::getColorCorrectionMatrix();
if (ccm.isValid) {
    Serial.printf("Matrix determinant: %f\n", ccm.determinant);
}
```

## Troubleshooting

### Common Issues

1. **"Matrix is singular"**
   - Check for duplicate calibration points
   - Ensure sensor readings are not all identical

2. **"Sensor reading saturated"**
   - Reduce sensor gain or exposure time
   - Check for overexposure

3. **"Need at least 3 calibration points"**
   - Add more reference colors
   - Ensure all 5 colors are calibrated for best results

### Debug Mode

Enable debug endpoints:
```cpp
endpoints.setDebugMode(true);
```

Then access:
- `GET /api/calibration-debug` for detailed information

## Performance

- **Memory Usage**: ~2KB for calibration data
- **Startup Time**: <100ms with cached data
- **Conversion Time**: <1ms per color conversion
- **Matrix Calculation**: <10ms for 5 points

## License

MIT License - See LICENSE file for details
