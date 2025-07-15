# ProS3 Battery Monitoring Implementation Guide

## 🔋 Overview

This guide covers the battery monitoring implementation for the **Unexpected Maker ESP32-S3 ProS3** board. The ProS3 has a built-in battery voltage monitoring circuit that allows you to read the battery voltage through GPIO1.

## 📋 Hardware Details

### ProS3 Battery Monitoring Circuit
- **GPIO Pin**: GPIO1 (ADC1_CH0)
- **Voltage Divider**: 2:1 ratio (built into ProS3 hardware)
- **ADC Resolution**: 12-bit (0-4095)
- **ADC Range**: 0-3.3V with 11dB attenuation
- **Battery Voltage Range**: 3.0V - 4.2V (typical LiPo)

### Circuit Explanation
```
Battery (+) ----[R1]----+----[R2]----GND
                        |
                     GPIO1 (ADC)
```

The ProS3 has a 2:1 voltage divider, meaning:
- Battery at 4.2V → GPIO1 reads 2.1V
- Battery at 3.7V → GPIO1 reads 1.85V
- Battery at 3.0V → GPIO1 reads 1.5V

## 🔧 Implementation

### Current Implementation
```cpp
// Battery monitoring for ProS3 - GPIO1 is connected to battery voltage divider
#define BATTERY_PIN 1  // GPIO1 - ProS3 battery voltage pin
#define BATTERY_VOLTAGE_DIVIDER 2.0f  // ProS3 has 2:1 voltage divider

float getBatteryVoltage() {
  // Configure ADC for battery reading - ProS3 specific settings
  analogSetAttenuation(ADC_11db);  // 0-3.3V range for higher voltages
  analogReadResolution(12);        // 12-bit resolution (0-4095)
  
  // Take multiple readings for accuracy and stability
  const int readings = 10;
  uint32_t total = 0;
  for (int i = 0; i < readings; i++) {
    total += analogRead(BATTERY_PIN);
    delay(2);  // Small delay between readings
  }
  uint32_t average = total / readings;
  
  // ProS3 battery voltage calculation
  // ADC reading -> voltage with 3.3V reference -> actual battery voltage through 2:1 divider
  float adcVoltage = (average / 4095.0f) * 3.3f;
  float batteryVoltage = adcVoltage * BATTERY_VOLTAGE_DIVIDER;
  
  return batteryVoltage;
}
```

### API Endpoints

#### Battery Status API
```bash
GET /api/battery
```

**Response:**
```json
{
  "batteryVoltage": 3.85,
  "timestamp": 12345,
  "source": "adc_gpio1",
  "status": "good",
  "percentage": 75
}
```

#### Battery Status Interpretation
- **excellent**: > 4.0V (90-100%)
- **good**: 3.7V - 4.0V (50-89%)
- **low**: 3.4V - 3.7V (10-49%)
- **critical**: < 3.4V (0-9%)

## 🌐 Web Interface Integration

### Battery Display
The web interface shows a compact battery indicator in the header:
- **Battery voltage** displayed in real-time
- **Color-coded icon** based on charge level
- **Updates every 10 seconds** via API polling

### JavaScript Implementation
```javascript
// Battery monitoring
function updateBatteryDisplay() {
    fetch('/api/battery')
        .then(response => response.json())
        .then(data => {
            const batteryVoltage = document.getElementById('batteryVoltage');
            const batteryIcon = document.getElementById('batteryIcon');
            
            if (batteryVoltage && batteryIcon) {
                batteryVoltage.textContent = data.batteryVoltage.toFixed(2) + 'V';
                
                // Update icon based on status
                if (data.status === 'excellent') {
                    batteryIcon.textContent = '🔋';
                } else if (data.status === 'good') {
                    batteryIcon.textContent = '🔋';
                } else if (data.status === 'low') {
                    batteryIcon.textContent = '🪫';
                } else {
                    batteryIcon.textContent = '🪫';
                }
            }
        })
        .catch(error => console.error('Battery update error:', error));
}

// Update battery every 10 seconds
setInterval(updateBatteryDisplay, 10000);
```

## 🔍 Troubleshooting

### Common Issues

#### 1. Reading 0.1V or Very Low Voltage
**Symptoms**: Battery shows 0.1V when it should be 3.7V+
**Causes**:
- ADC not properly configured
- Wrong GPIO pin
- Hardware connection issue
- Voltage divider calculation error

**Solutions**:
- Ensure ADC_11db attenuation is set
- Verify GPIO1 is the correct pin for ProS3
- Check battery is properly connected
- Verify 2:1 voltage divider constant

#### 2. Unstable Readings
**Symptoms**: Battery voltage fluctuates significantly
**Causes**:
- Insufficient averaging
- ADC noise
- Power supply instability

**Solutions**:
- Increase number of averaged readings
- Add delay between readings
- Check power supply quality

#### 3. Incorrect Voltage Calculation
**Symptoms**: Voltage is consistently wrong by a factor
**Causes**:
- Wrong voltage divider ratio
- ADC reference voltage incorrect
- Resolution settings wrong

**Solutions**:
- Verify ProS3 uses 2:1 voltage divider
- Confirm 3.3V ADC reference
- Ensure 12-bit resolution (0-4095)

### Debugging Commands

```bash
# Check current battery status
curl "http://192.168.0.152/api/battery"

# Monitor battery in real-time
while true; do curl -s "http://192.168.0.152/api/battery" | jq '.batteryVoltage, .status'; sleep 2; done

# Check color API (includes battery voltage)
curl "http://192.168.0.152/api/color"
```

### Serial Monitor Debug Output
```
[BATTERY] ProS3 GPIO1 ADC: 2500 (2.010V) -> Battery: 4.020V
[INFO] Battery voltage: 4.020V
```

## 📚 References

- [Unexpected Maker ESP32-S3 Arduino Helper Library](https://github.com/UnexpectedMaker/esp32s3-arduino-helper)
- [Unexpected Maker ESP32-S3 Resources](https://github.com/UnexpectedMaker/esp32s3)
- [ProS3 Official Documentation](https://unexpectedmaker.com/shop/pros3)
- [ESP32-S3 ADC Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/adc.html)

## 🎯 Best Practices

1. **Always average multiple readings** for stable results
2. **Use appropriate ADC attenuation** (11dB for 0-3.3V range)
3. **Set correct resolution** (12-bit for ESP32-S3)
4. **Add delays between readings** to reduce noise
5. **Monitor battery status** to prevent over-discharge
6. **Use proper voltage divider calculation** (2:1 for ProS3)

## 🔄 Alternative Implementations

### Using Official Helper Library
If you want to use the official Unexpected Maker helper library:

```cpp
#include <UMProS3.h>

UMProS3 board;

void setup() {
    board.begin();
}

float getBatteryVoltage() {
    return board.getBatteryVoltage();
}
```

### Manual ADC Reading (Current Implementation)
The current implementation manually reads the ADC and applies the voltage divider calculation, which gives you more control and doesn't require additional libraries.

## 📈 Performance Considerations

- **Reading frequency**: Don't read too frequently (every 10 seconds is sufficient)
- **Averaging**: 10 readings provides good balance of accuracy vs speed
- **Memory usage**: Minimal impact on system resources
- **Power consumption**: ADC readings consume minimal power

This implementation provides reliable battery monitoring for the ProS3 board while maintaining compatibility with the existing color matcher system. 