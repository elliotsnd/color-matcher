# Runtime Calibration Guide

## 🎯 Overview

Your ESP32 color matcher now supports **runtime calibration adjustments** without requiring firmware uploads. You can dynamically tune the sensor to achieve accurate color matching for specific target colors like **Vivid White (247,248,244)** and **Grey Port (168,160,147)**.

## 🚀 Quick Start

1. **Access the Web Interface**
   - Connect to your WiFi network
   - Open your browser and go to: `http://192.168.0.152`
   - The web interface will load with live color detection

2. **New Calibration Controls**
   - Scroll down to see the **"🎯 Quadratic Calibration"** section
   - Find **"🎯 Quick Target Tuning"** buttons for instant optimization

## 🔧 Calibration Features

### 📊 Quadratic Coefficients (Advanced)

Each color channel uses a quadratic equation: **RGB = A×XYZ² + B×XYZ + C**

**Red Channel (R = A×X² + B×X + C)**
- **Red A**: Quadratic coefficient (×10⁻⁶)
- **Red B**: Linear coefficient  
- **Red C**: Constant offset

**Green Channel (G = A×Y² + B×Y + C)**
- **Green A**: Quadratic coefficient (×10⁻⁶)
- **Green B**: Linear coefficient
- **Green C**: Constant offset

**Blue Channel (B = A×Z² + B×Z + C)**
- **Blue A**: Quadratic coefficient (×10⁻⁶)
- **Blue B**: Linear coefficient
- **Blue C**: Constant offset

### 🎯 Quick Target Tuning

**One-Click Optimization Buttons:**

1. **"Tune for Vivid White (247,248,244)"**
   - Instantly optimizes all coefficients for accurate white detection
   - Adjusts brightness and color balance for bright whites

2. **"Tune for Grey Port (168,160,147)"**
   - Optimizes for mid-tone gray colors
   - Balances coefficients for accurate gray detection

## 📝 How to Use

### Method 1: Quick Tuning (Recommended)

1. **For Vivid White Colors:**
   ```
   1. Place a Vivid White sample under the sensor
   2. Click "Tune for Vivid White (247,248,244)"
   3. Watch the live color reading adjust
   4. Fine-tune individual coefficients if needed
   ```

2. **For Grey Port Colors:**
   ```
   1. Place a Grey Port sample under the sensor
   2. Click "Tune for Grey Port (168,160,147)"
   3. Observe the color accuracy improvement
   4. Make manual adjustments if required
   ```

### Method 2: Manual Coefficient Adjustment

1. **Adjust Individual Coefficients:**
   - Modify Red, Green, or Blue A/B/C values
   - Click **"Apply Calibration"** to update the sensor
   - Monitor the live color feed for changes

2. **Real-Time Feedback:**
   - All changes apply instantly
   - Watch the **Live View** for immediate results
   - RGB values update in real-time

### Method 3: Fine-Tuning

1. **Start with Quick Tuning:**
   - Use the target-specific buttons first
   - This gets you 90% of the way to accurate colors

2. **Manual Fine-Tuning:**
   - Adjust individual coefficients for precision
   - Small changes in A coefficients have big effects
   - B and C coefficients provide fine control

## 💡 Calibration Tips

### 🎨 Understanding the Coefficients

- **A coefficients**: Control the curve shape (parabolic response)
  - Positive A = upward curve (brightens at high values)
  - Negative A = downward curve (dims at high values)

- **B coefficients**: Control linear response
  - Positive B = increases brightness linearly
  - Negative B = decreases brightness linearly

- **C coefficients**: Control baseline offset
  - Higher C = brighter overall
  - Lower C = darker overall

### 🔧 Adjustment Strategy

1. **Use Quick Tuning First**: Start with the target-specific buttons
2. **Monitor Live Feed**: Watch RGB values change in real-time
3. **Small Increments**: Make small adjustments (0.1 for A, 0.001 for B, 1 for C)
4. **Test Multiple Samples**: Try different shades of your target color
5. **Save Settings**: Settings persist automatically

### 📊 Coefficient Ranges

**Typical Working Ranges:**
- **A coefficients**: -5.0 to +10.0 (×10⁻⁶)
- **B coefficients**: -0.2 to +0.1
- **C coefficients**: 0 to 1000

### 🔍 Troubleshooting

**Colors Too Bright:**
- Decrease C coefficients
- Adjust B coefficients negative
- Reduce A coefficients if very bright

**Colors Too Dark:**
- Increase C coefficients
- Adjust B coefficients positive
- Check IR compensation factors

**Wrong Color Balance:**
- Use target-specific tuning buttons
- Adjust individual channel coefficients
- Check LED brightness settings

## 🔄 Reset Options

1. **Reset Calibration to Defaults:**
   - Click **"Reset to Defaults"** in calibration section
   - Restores factory quadratic coefficients

2. **Reset All Settings:**
   - Click **"Reset to Defaults"** in main settings
   - Resets all parameters including calibration

## 📱 Web Interface Features

### 🔧 Real-Time Settings
- **LED Brightness**: 0-255 (affects all readings)
- **Integration Time**: Sensor measurement duration
- **IR Compensation**: IR1/IR2 factors for environmental compensation
- **Color Samples**: Number of readings to average
- **Sample Delay**: Time between readings

### 🎯 Calibration Controls
- **Quadratic Coefficients**: A, B, C for each RGB channel
- **Quick Tuning**: One-click optimization buttons
- **Apply/Reset**: Manual control buttons

### 🐛 Debug Options
- **Show Sensor Details**: Raw XYZ and conversion data
- **Show Color Matching**: Database search results

## 📈 Advanced Usage

### 🎛️ Creating Custom Profiles

1. **Measure Your Target Color:**
   - Place sample under sensor
   - Note the current RGB reading
   - Note your desired RGB target

2. **Calculate Adjustments:**
   - If reading is too low, increase C coefficients
   - If reading is too high, decrease C coefficients
   - Use B coefficients for fine-tuning

3. **Test and Iterate:**
   - Apply changes incrementally
   - Test with multiple samples
   - Save successful configurations

### 🔬 Understanding Color Science

The quadratic calibration formula:
```
R = A_R × X² + B_R × X + C_R
G = A_G × Y² + B_G × Y + C_G  
B = A_B × Z² + B_B × Z + C_B
```

This accounts for:
- **Non-linear sensor response** (A coefficient)
- **Linear adjustments** (B coefficient)  
- **Baseline corrections** (C coefficient)
- **IR interference** (separate IR compensation)

## 🎉 Benefits

✅ **No Firmware Uploads**: All adjustments via web interface
✅ **Real-Time Feedback**: See changes instantly
✅ **Target-Specific Tuning**: Optimized for your exact colors
✅ **Persistent Settings**: Saves automatically
✅ **Professional Accuracy**: Quadratic correction for precision
✅ **Multiple Methods**: Quick tuning + manual fine-tuning

## 🆘 Support

If you need help:
1. Check the **Debug Options** for detailed sensor data
2. Use **"Reset to Defaults"** if settings get corrupted
3. Try the target-specific tuning buttons first
4. Monitor the console output for diagnostic information

---

**Happy Color Matching! 🎨**

Your ESP32 can now be tuned for any target color without code changes!
