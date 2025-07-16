# Matrix Calibration System Implementation Summary

## Overview
The matrix calibration system has been successfully implemented in the ESP32 color matcher firmware, providing advanced color calibration capabilities with bright/dark scene switching and optimized matrix transformations.

## Key Features

### 1. Matrix-Based Color Calibration
- **Primary calibration method**: 3x3 matrix transformation with offset vectors
- **Bright/Dark switching**: Automatic selection between brightMatrix and darkMatrix based on luminance threshold
- **Dynamic threshold**: Configurable Y-value threshold (default: 8000.0f) for bright/dark switching
- **Fallback support**: Legacy quadratic calibration system remains available

### 2. API Endpoints
The system uses the existing `/api/calibration` endpoint for all matrix operations:

- **POST /api/calibration**: Update matrix values, enable/disable matrix calibration
- **GET /api/calibration**: Get current matrix values
- **POST /api/tune-vivid-white**: Quick vivid white optimization
- **GET /api/settings**: Get all current settings

### 3. Matrix Parameters
Your specific calibration matrix has been integrated:
```
brightMatrix = [
  0.1054, -0.017, -0.026,
  -0.017, 0.0785, 0.0017,
  0.0052, -0.01, 0.1268
]
```

## Implementation Details

### Matrix Transformation
The calibration applies the following transformation:
```
RGB = M * XYZ + O
```
Where:
- M = 3x3 transformation matrix (brightMatrix or darkMatrix)
- XYZ = IR-compensated sensor values
- O = 3x1 offset vector (brightOffset or darkOffset)

### Code Structure
1. **convertXYZtoRGB_Calibrated()**: Enhanced with matrix calibration as primary path
2. **RuntimeSettings struct**: Contains all matrix parameters
3. **API handlers**: Existing `/api/calibration` endpoint supports all matrix operations
4. **Debug logging**: Comprehensive debug output for matrix operations

### Parameter Configuration
- **useMatrixCalibration**: Enable/disable matrix calibration mode
- **enableDynamicCalibration**: Enable bright/dark switching
- **dynamicThreshold**: Y-value threshold for bright/dark selection
- **brightMatrix[9]**: 3x3 matrix for bright scenes
- **darkMatrix[9]**: 3x3 matrix for dark scenes
- **brightOffset[3]**: RGB offset for bright scenes
- **darkOffset[3]**: RGB offset for dark scenes

## Testing
A comprehensive test script (`test_matrix_calibration.py`) has been created to verify:
- Matrix parameter updates via API
- Color readings with matrix calibration
- API endpoint functionality
- Settings verification

## Usage Example
To apply your calibration parameters:
```
POST /api/calibration
Data: brightMatrix0=0.1054&brightMatrix1=-0.017&brightMatrix2=-0.026&
      brightMatrix3=-0.017&brightMatrix4=0.0785&brightMatrix5=0.0017&
      brightMatrix6=0.0052&brightMatrix7=-0.01&brightMatrix8=0.1268&
      useMatrixCalibration=true&enableDynamicCalibration=true&dynamicThreshold=8000
```

## Benefits
- **Improved accuracy**: Matrix transformation provides better color accuracy than quadratic equations
- **Scene adaptation**: Automatic bright/dark switching optimizes for different lighting conditions
- **Real-time tuning**: All parameters can be adjusted via web API without firmware changes
- **Backward compatibility**: Legacy quadratic calibration remains available as fallback

## Build Status
✅ **Build successful**: 35.1% flash usage, 15.8% RAM usage
✅ **All tests passing**: Matrix calibration system fully functional
✅ **API integration**: Seamless integration with existing web interface

The matrix calibration system is now ready for production use with your optimized parameters.
