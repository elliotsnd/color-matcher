# ProS3 Battery Monitoring Troubleshooting Guide

## 🔋 Current Issue: Low Battery Readings (0.15V)

You're experiencing very low battery voltage readings (0.15V) instead of the expected 3.7V from your LiPo battery. This guide will help you diagnose and fix the issue.

## 🔍 Diagnostic Steps

### 1. **Hardware Verification**

#### Check Battery Connection
- ✅ **Battery is connected** to VBAT terminal
- ✅ **Battery is turned on** (you mentioned turning it on)
- ❓ **Battery voltage check**: Use a multimeter to measure battery voltage directly
  - Expected: 3.0V - 4.2V for LiPo battery
  - If below 3.0V, battery needs charging

#### Check ProS3 Board
- ✅ **USB power connected** (device is responding to API calls)
- ❓ **VBAT pin continuity**: Check if VBAT pin is properly connected to GPIO1
- ❓ **Voltage divider circuit**: ProS3 should have 2:1 voltage divider on battery monitoring

### 2. **Software Verification**

#### Current Implementation Status
Your current code is using:
```cpp
#define BATTERY_PIN 1  // GPIO1 - ProS3 battery voltage pin
#define BATTERY_VOLTAGE_DIVIDER 2.0f  // ProS3 has 2:1 voltage divider
```

#### Expected vs Actual Readings
- **Expected ADC reading**: ~1800-2000 (for 3.7V battery through 2:1 divider)
- **Actual ADC reading**: ~80-150 (very low)
- **This suggests**: GPIO1 is not reading the battery voltage

## 🛠️ Troubleshooting Solutions

### Solution 1: Verify Correct GPIO Pin

The ProS3 board might use a different GPIO pin for battery monitoring. Try these alternatives:

#### Test Different GPIO Pins
```cpp
// Try these pins one by one:
#define BATTERY_PIN 4   // Alternative 1
#define BATTERY_PIN 5   // Alternative 2  
#define BATTERY_PIN 6   // Alternative 3
#define BATTERY_PIN 7   // Alternative 4
```

#### How to Test:
1. Change `BATTERY_PIN` in code
2. Compile and upload
3. Check API response for reasonable voltage reading

### Solution 2: Verify Voltage Divider Ratio

The voltage divider might not be exactly 2:1. Try these ratios:

```cpp
#define BATTERY_VOLTAGE_DIVIDER 1.0f   // No divider
#define BATTERY_VOLTAGE_DIVIDER 1.5f   // 1.5:1 ratio
#define BATTERY_VOLTAGE_DIVIDER 3.0f   // 3:1 ratio
```

### Solution 3: ADC Configuration Issues

Try different ADC configurations:

```cpp
// In getBatteryVoltage() function:
analogSetAttenuation(ADC_6db);   // Try 6dB instead of 11dB
analogSetAttenuation(ADC_2_5db); // Try 2.5dB
analogSetAttenuation(ADC_0db);   // Try 0dB
```

### Solution 4: Hardware Alternative - Use Official Helper Library

Install the official Unexpected Maker helper library:

```bash
pio lib install "https://github.com/UnexpectedMaker/esp32s3-arduino-helper.git"
```

Then use:
```cpp
#include "UMP3S.h"

void setup() {
  UMP3S.begin();
}

float getBatteryVoltage() {
  return UMP3S.getBatteryVoltage();
}
```

## 🔧 Immediate Testing Steps

### Step 1: Manual Voltage Check
```bash
# Test current API
curl "http://192.168.0.152/api/battery"

# Expected low reading: ~0.15V
# Goal: Get 3.0V - 4.2V reading
```

### Step 2: Try Different GPIO Pin
1. Edit `src/main.cpp`
2. Change `#define BATTERY_PIN 1` to `#define BATTERY_PIN 4`
3. Upload and test
4. If still low, try GPIO 5, 6, 7, etc.

### Step 3: Check ProS3 Documentation
- Visit: https://unexpectedmaker.com/shop/pros3
- Look for official battery monitoring pin specification
- Check schematic for actual voltage divider ratio

## 📊 Expected Results

### Good Battery Reading Example:
```json
{
  "batteryVoltage": 3.85,
  "status": "good",
  "percentage": 75,
  "source": "adc_gpio1"
}
```

### Current Problem Reading:
```json
{
  "batteryVoltage": 0.151,
  "status": "critical", 
  "percentage": 0,
  "source": "adc_gpio1_debug"
}
```

## 🚨 Safety Notes

- **Never exceed 4.2V** on LiPo batteries
- **Don't discharge below 3.0V** (can damage battery)
- **Check battery temperature** if charging/discharging rapidly
- **Use proper LiPo charging practices**

## 📞 Next Steps

1. **Measure battery voltage directly** with multimeter
2. **Try different GPIO pins** systematically
3. **Check ProS3 official documentation** for correct pin
4. **Consider using official helper library** if available
5. **Test with known good battery** if possible

## 🔄 Code Updates Ready

I've prepared updated code with:
- ✅ Clean `getBatteryVoltage()` function
- ✅ Proper `getVbusPresent()` function  
- ✅ Removed GPIO34 error (invalid pin)
- ✅ Better API response format

**Ready to upload when COM port is available!**

---

*This guide will help you systematically diagnose and fix the battery monitoring issue. The most likely cause is using the wrong GPIO pin for battery monitoring on your specific ProS3 board.* 