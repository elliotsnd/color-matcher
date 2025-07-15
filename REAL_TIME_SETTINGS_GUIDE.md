# Real-Time Settings Implementation - Summary

## 🎉 What's New

Your color matcher now supports **real-time settings adjustment** without needing to recompile and upload code! You can now adjust sensor parameters, LED brightness, and calibration values instantly through the web interface.

## 🔧 Features Added

### 1. **Runtime Settings Structure**
- All settings from `sensor_settings.h` are now available as runtime variables
- Changes take effect immediately without recompilation
- Settings are stored in memory and persist until device restart

### 2. **Battery Monitoring**
- **Real-time Battery Voltage Display**: Shows current battery voltage at the top of the interface
- **Battery Status Indicator**: Color-coded battery icon (green/yellow/orange/red)
- **Battery Percentage**: Estimated battery level based on voltage
- **I2C Fuel Gauge**: Uses ProS3's built-in fuel gauge for accurate readings
- **API Access**: Battery data available via `/api/battery` endpoint

### 3. **Web Interface Settings Panel**
- **LED Brightness Slider**: Adjust LED brightness 0-255 with instant feedback
- **Integration Time Selector**: Choose sensor timing from ultra-fast to ultra-precise
- **Color Sampling Controls**: Adjust sample count and delay timing
- **IR Compensation Sliders**: Fine-tune IR1 and IR2 compensation factors
- **Debug Toggles**: Enable/disable detailed sensor and color matching logs

### 4. **API Endpoints**
- `GET /api/settings` - Retrieve current settings
- `POST /api/settings` - Update multiple settings at once
- `POST /api/settings/led-brightness?value=X` - Instant LED brightness update
- `POST /api/settings/integration-time?value=X` - Instant sensor timing update
- `POST /api/settings/ir-compensation?ir1=X&ir2=Y` - Instant IR compensation update
- `GET /api/battery` - Get current battery voltage and status

## 🚀 How to Use

### 1. **Access the Settings Panel**
1. Open your color matcher's web interface (default: http://192.168.0.152)
2. At the top, you'll see the battery voltage display showing current power level
3. Scroll down to the "Real-Time Settings" panel
4. You'll see organized groups of settings with sliders and dropdowns

### 2. **Real-Time Adjustments**
- **LED Brightness**: Move the slider to instantly change LED brightness
- **Integration Time**: Select from dropdown for immediate sensor timing changes
- **IR Compensation**: Adjust sliders to fine-tune color accuracy in real-time
- **Other Settings**: Modify values and click "Apply Settings" to save

### 3. **Settings Management**
- **Load Current**: Refresh settings from the device
- **Apply Settings**: Save all modified settings to the device
- **Reset to Defaults**: Restore factory default values

## 📊 Settings Categories

### 🔆 LED & Sensor
- **LED Brightness** (0-255): Immediate brightness adjustment
- **Integration Time**: Sensor sampling speed vs accuracy tradeoff

### 🎨 Color Processing  
- **Color Samples** (1-10): Number of readings to average
- **Sample Delay** (1-20ms): Delay between individual samples

### 🔧 IR Compensation
- **IR1 Factor** (0-1): IR1 channel compensation strength
- **IR2 Factor** (0-1): IR2 channel compensation strength

### 🐛 Debug Options
- **Show Sensor Details**: Enable detailed XYZ→RGB conversion logs
- **Show Color Matching**: Enable color search result logging

## 🎯 Key Benefits

1. **No More Recompilation**: Adjust settings instantly without code changes
2. **Interactive Tuning**: See effects immediately while testing colors
3. **Easy Calibration**: Fine-tune IR compensation and other parameters live
4. **Debugging Control**: Toggle debug output on/off as needed
5. **Quick Reset**: Return to known good defaults with one click

## 🔄 Technical Implementation

### Code Changes Made:
1. **RuntimeSettings struct**: Replaced compile-time constants with runtime variables
2. **Settings API**: Added REST endpoints for getting/setting configuration
3. **Real-time Updates**: LED brightness, integration time, and IR compensation apply instantly
4. **Web Interface**: Added comprehensive settings panel with modern UI
5. **Memory Management**: Efficient settings storage and updates

### Settings That Apply Instantly:
- LED brightness (via `analogWrite`)
- Sensor integration time (via `TCS3430.setIntegrationTime`)
- IR compensation factors (used in color conversion)
- Debug output levels

### Settings That Apply on Next Reading:
- Color sampling count and delays
- Other calibration parameters

## 🛠️ Troubleshooting

### If Settings Don't Apply:
1. Check browser console for JavaScript errors
2. Verify ESP32 is connected to WiFi
3. Use "Load Current" to refresh from device
4. Try "Reset to Defaults" if settings seem corrupted

### For Color Accuracy Issues:
1. Start with default settings
2. Adjust IR compensation factors gradually
3. Try different integration times for your lighting conditions
4. Use debug output to monitor XYZ values

## 📚 Advanced Usage

### Direct API Access:
```bash
# Get current settings
curl http://192.168.0.152/api/settings

# Update LED brightness instantly  
curl -X POST "http://192.168.0.152/api/settings/led-brightness?value=150"

# Update integration time
curl -X POST "http://192.168.0.152/api/settings/integration-time?value=64"

# Get battery status
curl http://192.168.0.152/api/battery
```

### Integration Time Values:
- `0x00` (0): 2.78ms - Ultra fast
- `0x23` (35): 100ms - Balanced (default)
- `0x40` (64): 181ms - High precision
- `0x80` (128): 359ms - Very precise
- `0xFF` (255): 712ms - Maximum precision

This implementation gives you the flexibility to tune your color matcher's performance in real-time, making calibration and optimization much easier!
a full guide

An advanced ESP32-based color matching system that identifies colors using a TCS3430 sensor and matches them against a comprehensive Dulux color database. Features a modern web interface and highly optimized binary color database for memory efficiency.

✨ Features
🔍 Real-time Color Detection - TCS3430 sensor with XYZ to RGB conversion
🎯 Accurate Color Matching - 4,224+ Dulux colors with precise matching algorithms
💾 Memory Optimized - Binary color database (83% smaller than JSON)
🌐 Web Interface - Modern React-based UI with live color updates
📱 Responsive Design - Works on desktop and mobile devices
💡 Smart Caching - Efficient color lookup with result caching
🔧 Robust Architecture - Memory-safe streaming database access
🚀 Quick Start
I'll analyze the web interface to identify all the adjustable settings and provide you with the complete list of API endpoints that allow real-time control from the command line. Based on my analysis of the codebase, here's the complete list of API endpoints for real-time command line control of all adjustable settings:

🎯 Complete ESP32 Color Matcher API Endpoints
To achieve the most accurate color readings from your sensor, it's crucial to adjust its settings in a specific and logical order. The process moves from foundational hardware settings to more nuanced software corrections. Following this sequence ensures that each adjustment builds upon a stable and optimized base.
Here is the recommended order for adjusting your color sensor's settings using the provided API endpoints:

Foundational Setup: Illumination and Basic Sensitivity The first and most critical step is to establish a consistent lighting environment and configure the sensor's basic sensitivity. The goal is to get a strong, stable signal without overwhelming the sensor (saturation). For this, you will primarily use a neutral white or light gray reference object. Set LED Brightness: Begin by setting the illumination. The brightness should be sufficient to clearly light the object you intend to measure but not so bright that it causes reflections or saturation. API Endpoint: GET /api/set-led-brightness?value={0-255} Example: curl "http://192.168.0.152/api/set-led-brightness?value=100" Adjust Integration Time and Gain: These two settings work together to control the sensor's sensitivity to light. Integration Time: This is akin to a camera's shutter speed. A longer time allows more light to be gathered, which is ideal for darker objects. Start with a mid-range value. API Endpoint: GET /api/set-integration-time?value={0-255} Example: curl "http://192.168.0.152/api/set-integration-time?value=55" ALS Gain & High Gain: This amplifies the signal from the light sensor. Higher gain is beneficial for low-light conditions or when trying to distinguish between very similar colors. API Endpoint: GET /api/set-advanced-sensor?alsGain={0-3}&highGain={true/false} Example: curl "http://192.168.0.152/api/set-advanced-sensor?alsGain=2" Pro-Tip: A good method for this step is to place a white reference object in front of the sensor. Adjust the integration time and gain until the raw r, g, and b values from the /api/color endpoint are as high as possible without reaching the maximum value (which would indicate saturation). This maximizes the dynamic range of the sensor.[1]
Signal Stabilization: Averaging Samples Once you have a good, strong signal, you can improve the stability and repeatability of the readings by taking multiple samples and averaging them. This will help to reduce noise from the sensor and the environment. Set Color Samples: Determine the number of readings you want to average for each color measurement. More samples will yield a more stable result but will take longer. API Endpoint: GET /api/set-color-samples?value={1-10} Example: curl "http://192.168.0.152/api/set-color-samples?value=5" Set Sample Delay: Define the brief pause between each sample. A small delay can sometimes improve accuracy. API Endpoint: GET /api/set-sample-delay?value={1-50ms} Example: curl "http://192.168.0.152/api/set-sample-delay?value=2"
Refining Accuracy: IR Compensation Many light sources, including ambient light, can contain an infrared (IR) component that can interfere with the accuracy of color readings.[2] The TCS3430 sensor has features to help mitigate this. Set IR Compensation Factors: These factors are used to subtract the influence of infrared light from your color readings. Determining the optimal values may require some experimentation. One approach is to measure a material that is known to reflect IR light and adjust the factors to minimize its impact on the color measurement. API Endpoint: GET /api/set-ir-factors?ir1={0.0-2.0}&ir2={0.0-2.0} Example: curl "http://192.168.0.152/api/set-ir-factors?ir1=0.35&ir2=0.34"
Advanced Correction: Quadratic Calibration This is the final and most advanced step for achieving the highest color accuracy. Quadratic calibration applies a mathematical correction to the raw RGB values, compensating for any non-linear responses in the sensor.[3][4] This step is crucial when you need to match colors to a specific standard, like the CIE 1931 color space.[5][6] Set Quadratic Calibration Coefficients: To use this feature effectively, you would typically measure a set of standardized color references (like a color calibration chart). You would then use a separate program to calculate the ideal quadratic coefficients that map the sensor's readings to the known values of your color standards. These calculated coefficients are then sent to the device. API Endpoint: GET /api/set-quadratic-calibration?r_a=...&r_b=...&r_c=...&g_a=...&g_b=...&g_c=...&b_a=...&b_b=...&b_c=... Example: curl "http://192.168.0.152/api/set-quadratic-calibration?r_a=5.756615e-06&r_b=-0.108&r_c=663.2&..." After completing this process, remember to save your optimized settings to the device's persistent storage so they are not lost on restart. Save Settings: GET /api/save-settings
📊 Core Data Endpoints
bash

Collapse

Unwrap

Run

Copy
# Live color data (polling endpoint)
GET /api/color
# Returns: {"r":123,"g":45,"b":67,"x":1000,"y":2000,"z":1500,"ir1":300,"ir2":250,"colorName":"Color Name","batteryVoltage":3.85,"timestamp":12345}

# Current settings dump
GET /api/settings
# Returns: Complete JSON with all current settings

# Battery monitoring
GET /api/battery
# Returns: {"batteryVoltage":3.85,"status":"good","percentage":75,"timestamp":12345}

# System debug/health check
GET /api/debug
# Returns: {"status":"ok","message":"ESP32 API is working","timestamp":12345}
🔆 LED & Basic Sensor Controls
bash

Collapse

Unwrap

Run

Copy
# LED brightness control (0-255)
GET /api/set-led-brightness?value=85
# Example: curl "http://192.168.0.152/api/set-led-brightness?value=120"

# Integration time (0-255 hex values)
GET /api/set-integration-time?value=55
# Example: curl "http://192.168.0.152/api/set-integration-time?value=35"
# Common values: 0=2.78ms, 35=100ms, 55=153ms, 255=712ms
🔧 IR Compensation
bash

Collapse

Unwrap

Run

Copy
# IR compensation factors (0.0-2.0)
GET /api/set-ir-factors?ir1=0.35&ir2=0.34
# Example: curl "http://192.168.0.152/api/set-ir-factors?ir1=0.40&ir2=0.38"
🎨 Color Processing
bash

Collapse

Unwrap

Run

Copy
# Number of color samples (1-10)
GET /api/set-color-samples?value=5
# Example: curl "http://192.168.0.152/api/set-color-samples?value=7"

# Sample delay between readings (1-50ms)
GET /api/set-sample-delay?value=2
# Example: curl "http://192.168.0.152/api/set-sample-delay?value=5"
🐛 Debug Options
bash

Collapse

Unwrap

Run

Copy
# Debug settings (true/false)
GET /api/set-debug?sensor=true&colors=false
# Example: curl "http://192.168.0.152/api/set-debug?sensor=true&colors=true"
📡 Advanced Sensor Controls (Complete TCS3430 Configuration)
bash

Collapse

Unwrap

Run

Copy
# All advanced sensor parameters in one call
GET /api/set-advanced-sensor?alsGain=3&highGain=false&waitTimer=true&waitLong=false&waitTime=10&autoZeroMode=1&autoZeroNTH=0&intPersistence=0&intReadClear=true&sleepAfterInt=false&alsInterrupt=false&alsSatInterrupt=false&ch0ThreshLow=0&ch0ThreshHigh=65535

# Individual parameter examples:
# ALS Gain (0-3: 1x, 4x, 16x, 64x)
curl "http://192.168.0.152/api/set-advanced-sensor?alsGain=2"

# High Gain Mode (128x multiplier)
curl "http://192.168.0.152/api/set-advanced-sensor?highGain=true"

# Wait Timer Controls
curl "http://192.168.0.152/api/set-advanced-sensor?waitTimer=true&waitLong=false&waitTime=50"

# Auto Zero Configuration
curl "http://192.168.0.152/api/set-advanced-sensor?autoZeroMode=1&autoZeroNTH=2"

# Interrupt System
curl "http://192.168.0.152/api/set-advanced-sensor?intPersistence=5&intReadClear=true&sleepAfterInt=false"

# Interrupt Types
curl "http://192.168.0.152/api/set-advanced-sensor?alsInterrupt=true&alsSatInterrupt=false"

# Channel Thresholds
curl "http://192.168.0.152/api/set-advanced-sensor?ch0ThreshLow=1000&ch0ThreshHigh=50000"
🎯 Quadratic Calibration (Real-time Color Correction)
bash

Collapse

Unwrap

Run

Copy
# Quadratic calibration coefficients (live RGB correction)
GET /api/set-quadratic-calibration?r_a=5.756615e-06&r_b=-0.108&r_c=663.2&g_a=7.700365e-06&g_b=-0.149&g_c=855.3&b_a=-2.758863e-06&b_b=0.050&b_c=35.6

# Individual channel examples:
# Red channel
curl "http://192.168.0.152/api/set-quadratic-calibration?r_a=6e-06&r_b=-0.1&r_c=650"

# Green channel  
curl "http://192.168.0.152/api/set-quadratic-calibration?g_a=8e-06&g_b=-0.15&g_c=860"

# Blue channel
curl "http://192.168.0.152/api/set-quadratic-calibration?b_a=-3e-06&b_b=0.05&b_c=40"
⚙️ Settings Management
bash

Collapse

Unwrap

Run

Copy
# Save current settings to persistent storage
GET /api/save-settings

# Reset all settings to factory defaults
GET /api/reset-defaults

# Clear all saved settings (requires restart)
GET /api/clear-settings
🚀 Command Line Usage Examples
Quick Color Detection Setup
bash

Collapse

Unwrap

Run

Copy
# Set optimal lighting and sensitivity for color detection
curl "http://192.168.0.152/api/set-led-brightness?value=90"

curl "http://192.168.0.152/api/set-integration-time?value=55"  # 153ms balanced

curl "http://192.168.0.152/api/set-advanced-sensor?alsGain=2"  # 16x sensitivity

curl "http://192.168.0.152/api/set-color-samples?value=7"
Low Light Configuration
bash

Collapse

Unwrap

Run

Copy
# Configure for dim lighting conditions
curl "http://192.168.0.152/api/set-led-brightness?value=150"

curl "http://192.168.0.152/api/set-advanced-sensor?alsGain=3&highGain=true"  # 64x + 128x = 8192x total

curl "http://192.168.0.152/api/set-integration-time?value=128"  # 359ms longer exposure
High Speed Scanning
bash

Collapse

Unwrap

Run

Copy
# Configure for rapid color detection
curl "http://192.168.0.152/api/set-integration-time?value=16"  # 47ms fast

curl "http://192.168.0.152/api/set-color-samples?value=3"

curl "http://192.168.0.152/api/set-sample-delay?value=1"

curl "http://192.168.0.152/api/set-advanced-sensor?alsGain=1"  # 4x moderate sensitivity
Precision Calibration Mode
bash

Collapse

Unwrap

Run

Copy
# Configure for maximum accuracy
curl "http://192.168.0.152/api/set-integration-time?value=192"  # 537ms high precision

curl "http://192.168.0.152/api/set-color-samples?value=10"

curl "http://192.168.0.152/api/set-sample-delay?value=10"

curl "http://192.168.0.152/api/set-debug?sensor=true&colors=true"
Real-time Monitoring Script
bash

Collapse

Unwrap

Run

Copy
#!/bin/bash
# Monitor live color data
while true; do
    curl -s "http://192.168.0.152/api/color" | jq '.r, .g, .b, .colorName'
    sleep 0.5
done
Batch Configuration
bash

Collapse

Unwrap

Run

Copy
#!/bin/bash
# Complete sensor setup script
IP="192.168.0.152"

echo "🔧 Configuring ESP32 Color Matcher..."

# Basic settings
curl -s "$IP/api/set-led-brightness?value=90" | jq '.status'

curl -s "$IP/api/set-integration-time?value=55" | jq '.status'

curl -s "$IP/api/set-ir-factors?ir1=0.36&ir2=0.35" | jq '.status'

# Advanced sensor configuration
curl -s "$IP/api/set-advanced-sensor?alsGain=2&highGain=false&waitTimer=false&autoZeroMode=1" | jq '.status'

# Color processing
curl -s "$IP/api/set-color-samples?value=5" | jq '.status'
curl -s "$IP/api/set-sample-delay?value=2" | jq '.status'

# Save settings
curl -s "$IP/api/save-settings" | jq '.status'

echo "✅ Configuration complete!"
📊 Live Effects Verification
All these endpoints provide immediate real-time effects that you can see in:

Live color readings via /api/color
Web interface updates automatically
Serial monitor output for debugging
Visual color display changes instantly
The system debounces rapid changes (500ms-1.5s) to prevent API flooding while maintaining responsive real-time control.

Hardware Requirements
ESP32-S3 (recommended: Unexpected Maker ProS3)
TCS3430 Color Sensor (DFRobot)
16MB Flash (for LittleFS filesystem)
PSRAM (for optimal performance)
Software Requirements
PlatformIO (recommended over Arduino IDE)
Node.js (for frontend development)
Python 3.7+ (for build scripts)
Installation
Clone the repository
bash




git clone <repository-url>
cd esp32-color-matcher