/**
 * @file test_register_mapping.cpp
 * @brief Test to verify the new register mapping fixes calibration issues
 * 
 * This test compares the old vs new register mapping to see if:
 * 1. X and IR1 channels are swapped (indicating the fix is working)
 * 2. White calibration gives better values
 * 3. Blue channel detection improves
 */

#include <Arduino.h>
#include <Wire.h>
#include "lib/TCS3430AutoGain/TCS3430AutoGain.h"

TCS3430AutoGain colorSensor;

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("=== TCS3430 REGISTER MAPPING TEST ===");
    Serial.println("Testing if corrected register mapping fixes calibration issues");
    Serial.println();
    
    // Initialize I2C
    Wire.begin(3, 4); // SDA=3, SCL=4 for ESP32-S3 ProS3
    
    // Initialize sensor
    if (!colorSensor.begin()) {
        Serial.println("ERROR: Sensor initialization failed!");
        Serial.println("Check I2C connections and sensor power");
        while (1) delay(1000);
    }
    
    Serial.println("✓ Sensor initialized successfully");
    
    // Configure sensor for optimal testing
    colorSensor.power(true);
    colorSensor.mode(TCS3430AutoGain::ALS);
    colorSensor.gain(TCS3430AutoGain::X16);  // 16x gain for good sensitivity
    colorSensor.integrationTime(150.0f);     // 150ms integration
    
    Serial.println("✓ Sensor configured: 16x gain, 150ms integration");
    Serial.println();
    
    delay(500); // Allow sensor to stabilize
    
    Serial.println("=== REGISTER MAPPING VERIFICATION ===");
    Serial.println("NEW MAPPING: X=0x9A, Y=0x96, Z=0x94, IR1=0x98, IR2=0x9C");
    Serial.println("OLD MAPPING: X=0x94, Y=0x96, Z=0x98, IR1=0x9A, IR2=0x9C");
    Serial.println();
    Serial.println("*** If X and IR1 values are different from before, the fix is working! ***");
    Serial.println();
}

void loop() {
    // Read sensor data with NEW register mapping
    TCS3430AutoGain::RawData data = colorSensor.raw();
    
    Serial.println("=== CURRENT READINGS (NEW MAPPING) ===");
    Serial.printf("X=%5d  Y=%5d  Z=%5d  IR1=%5d  IR2=%5d\n", 
                  data.X, data.Y, data.Z, data.IR1, data.IR2);
    
    // Test color calculations
    float lux = colorSensor.lux();
    float cct = colorSensor.colorTemp();
    float x_chroma = colorSensor.chromaticityX();
    float y_chroma = colorSensor.chromaticityY();
    
    Serial.printf("Lux: %.1f  CCT: %.0fK  Chromaticity: x=%.3f y=%.3f\n", 
                  lux, cct, x_chroma, y_chroma);
    
    // Simple RGB conversion test (basic XYZ to RGB)
    float X_norm = data.X / 65535.0f;
    float Y_norm = data.Y / 65535.0f; 
    float Z_norm = data.Z / 65535.0f;
    
    // Simple sRGB conversion matrix (approximate)
    float R = 3.2406f * X_norm - 1.5372f * Y_norm - 0.4986f * Z_norm;
    float G = -0.9689f * X_norm + 1.8758f * Y_norm + 0.0415f * Z_norm;
    float B = 0.0557f * X_norm - 0.2040f * Y_norm + 1.0570f * Z_norm;
    
    // Convert to 0-255 range
    int r = constrain((int)(R * 255), 0, 255);
    int g = constrain((int)(G * 255), 0, 255);
    int b = constrain((int)(B * 255), 0, 255);
    
    Serial.printf("Approximate RGB: R=%3d G=%3d B=%3d\n", r, g, b);
    
    // Test specific calibration scenarios
    Serial.println();
    Serial.println("=== CALIBRATION TEST SCENARIOS ===");
    
    // White test
    if (data.Y > 30000) {
        Serial.println("🔍 HIGH Y VALUE DETECTED - Testing white calibration:");
        Serial.printf("   Expected for white: R≈255, G≈255, B≈255\n");
        Serial.printf("   Current result:     R=%3d, G=%3d, B=%3d\n", r, g, b);
        if (r > 200 && g > 200 && b > 200) {
            Serial.println("   ✅ WHITE CALIBRATION LOOKS GOOD!");
        } else {
            Serial.println("   ❌ White calibration still needs work");
        }
    }
    
    // Blue test (Z channel should be high for blue)
    if (data.Z > data.X && data.Z > data.Y) {
        Serial.println("🔍 HIGH Z VALUE DETECTED - Testing blue detection:");
        Serial.printf("   Z channel dominance: Z=%d vs X=%d, Y=%d\n", data.Z, data.X, data.Y);
        if (b > r && b > g) {
            Serial.println("   ✅ BLUE CHANNEL DETECTION WORKING!");
        } else {
            Serial.println("   ❌ Blue detection still needs work");
        }
    }
    
    Serial.println();
    Serial.println("*** COMPARE THESE VALUES TO YOUR OLD CALIBRATION DATA ***");
    Serial.println("*** Key indicators of success: ***");
    Serial.println("*** 1. X and IR1 values should be swapped from before ***");
    Serial.println("*** 2. White objects should give RGB closer to (255,255,255) ***");
    Serial.println("*** 3. Blue objects should show higher B values ***");
    Serial.println();
    
    delay(3000);
}
