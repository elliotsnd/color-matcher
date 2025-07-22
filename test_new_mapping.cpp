/**
 * @file test_new_mapping.cpp
 * @brief Quick test to verify the new register mapping works
 */

#include <Arduino.h>
#include <Wire.h>
#include "lib/TCS3430AutoGain/TCS3430AutoGain.h"

TCS3430AutoGain colorSensor;

void setup() {
    Serial.begin(115200);
    Wire.begin(3, 4); // SDA=3, SCL=4 for ESP32-S3 ProS3
    
    Serial.println("=== TCS3430 Register Mapping Test ===");
    Serial.println("Testing if corrected register mapping fixes calibration issues");
    
    if (!colorSensor.begin()) {
        Serial.println("ERROR: Sensor initialization failed!");
        return;
    }
    
    Serial.println("Sensor initialized successfully");
    
    // Configure sensor
    colorSensor.power(true);
    colorSensor.mode(TCS3430AutoGain::ALS);
    colorSensor.gain(TCS3430AutoGain::X16);
    colorSensor.integrationTime(150.0f);
    
    Serial.println("Sensor configured: 16x gain, 150ms integration");
    delay(200);
}

void loop() {
    // Read with NEW register mapping
    TCS3430AutoGain::RawData data = colorSensor.raw();
    
    Serial.println("=== NEW REGISTER MAPPING ===");
    Serial.println("X=" + String(data.X) + " Y=" + String(data.Y) + " Z=" + String(data.Z) + 
                   " IR1=" + String(data.IR1) + " IR2=" + String(data.IR2));
    
    // Test color calculations
    float lux = colorSensor.lux();
    float cct = colorSensor.colorTemp();
    float x_chroma = colorSensor.chromaticityX();
    float y_chroma = colorSensor.chromaticityY();
    
    Serial.println("Lux: " + String(lux, 1) + " CCT: " + String(cct, 0) + "K");
    Serial.println("Chromaticity: x=" + String(x_chroma, 3) + " y=" + String(y_chroma, 3));
    
    Serial.println("*** COMPARE THESE VALUES TO OLD MAPPING ***");
    Serial.println("*** If X and IR1 values are swapped, the fix is working! ***");
    Serial.println();
    
    delay(2000);
}
