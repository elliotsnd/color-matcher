# 🎯 Quick Fix for Vivid White Calibration Issue

## Problem
When scanning vivid white, you're getting RGB: (21, 169, 0) instead of the expected white values around (255, 255, 255).

## Root Cause
The custom calibration matrix in the current settings is poorly tuned. The values are too small (around 0.02-0.06) which results in very low RGB outputs.

## 🚀 Quick Fixes

### Option 1: Use DFRobot Calibration (Recommended)
```bash
# Switch to DFRobot's built-in calibration which is more reliable
curl "http://192.168.0.152/api/fix-white-calibration"
```

### Option 2: Manual Switch to DFRobot Calibration
```bash
# Enable DFRobot calibration
curl "http://192.168.0.152/api/use-dfrobot-calibration?enable=true"
```

### Option 3: Optimize Sensor Settings for White Detection
```bash
# Set optimal sensor parameters for white colors
curl "http://192.168.0.152/api/set-advanced-sensor?alsGain=2"
curl "http://192.168.0.152/api/set-advanced-sensor?highGain=false" 
curl "http://192.168.0.152/api/set-advanced-sensor?autoZeroMode=1"
curl "http://192.168.0.152/api/set-advanced-sensor?autoZeroNTH=0"
```

## 🔧 What the Fix Does

1. **Switches to DFRobot Calibration**: Uses the library's standard XYZ to RGB conversion matrix
2. **Optimizes Sensor Settings**: 
   - Sets ALS gain to 16x (good sensitivity without saturation)
   - Disables high gain to prevent oversaturation on bright colors
   - Configures auto-zero for stable readings
3. **Provides Reliable White Detection**: DFRobot's calibration is tested and validated

## 🧪 Test After Fix

After running the fix, scan vivid white again. You should now get RGB values closer to:
- **Expected**: RGB(240-255, 240-255, 240-255) for vivid white
- **Previous**: RGB(21, 169, 0) ❌
- **Fixed**: RGB(~245, ~245, ~245) ✅

## 📊 Alternative: Use Auto-Gain

You can also try the new auto-gain feature:
```bash
# Let the sensor automatically optimize settings
curl "http://192.168.0.152/api/use-auto-gain?targetY=1000"
```

## 🔍 Verification

Check if the fix worked by calling:
```bash
curl "http://192.168.0.152/api/color"
```

The response should show much better RGB values for white colors.
