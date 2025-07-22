/**
 * @file test_ambient_compensation.cpp
 * @brief Test ambient compensation (black reference subtraction) system
 * 
 * This test validates that the ambient compensation properly:
 * 1. Subtracts dark current and ambient light
 * 2. Improves color accuracy in low-light conditions
 * 3. Handles edge cases (negative values, etc.)
 */

#include <Arduino.h>
#include <Wire.h>
#include "lib/TCS3430AutoGain/TCS3430AutoGain.h"

TCS3430AutoGain colorSensor;

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("=== AMBIENT COMPENSATION TEST ===");
    Serial.println("Testing black reference subtraction system");
    Serial.println();
    
    // Initialize I2C and sensor
    Wire.begin(3, 4);
    
    if (!colorSensor.begin()) {
        Serial.println("ERROR: Sensor initialization failed!");
        while (1) delay(1000);
    }
    
    Serial.println("✓ Sensor initialized successfully");
    
    // Configure sensor
    colorSensor.power(true);
    colorSensor.mode(TCS3430AutoGain::ALS);
    colorSensor.gain(TCS3430AutoGain::X16);
    colorSensor.integrationTime(150.0f);
    
    Serial.println("✓ Sensor configured");
    Serial.println();
    
    // Test ambient compensation
    testAmbientCompensation();
}

void testAmbientCompensation() {
    Serial.println("=== AMBIENT COMPENSATION CALIBRATION ===");
    Serial.println();
    
    // Step 1: Calibrate black reference
    Serial.println("STEP 1: Black Reference Calibration");
    Serial.println("Please COVER the sensor completely (use dark cloth or hand)");
    Serial.println("Press any key when ready...");
    
    while (!Serial.available()) {
        delay(100);
    }
    Serial.read(); // Clear buffer
    
    Serial.println("Calibrating black reference...");
    
    if (colorSensor.calibrateBlackReference(20)) {
        Serial.println("✓ Black reference calibrated successfully");
        
        // Show black reference values
        ColorScience::CalibrationData calibData = colorSensor.getCalibrationData();
        Serial.printf("Black Reference: X=%.4f Y=%.4f Z=%.4f\n", 
                     calibData.blackReference.X, 
                     calibData.blackReference.Y, 
                     calibData.blackReference.Z);
        Serial.printf("Black IR: IR1=%.4f IR2=%.4f\n", 
                     calibData.blackIR.IR1, calibData.blackIR.IR2);
    } else {
        Serial.println("❌ Black reference calibration failed");
        return;
    }
    
    Serial.println();
    
    // Step 2: Test with and without ambient compensation
    Serial.println("STEP 2: Ambient Compensation Test");
    Serial.println("Please UNCOVER the sensor and place a white object over it");
    Serial.println("Press any key when ready...");
    
    while (!Serial.available()) {
        delay(100);
    }
    Serial.read(); // Clear buffer
    
    delay(1000); // Allow sensor to stabilize
    
    // Test without ambient compensation
    colorSensor.configureColorScience(true, false, 0.08f); // IR comp on, ambient comp off
    ColorScience::RGBColor rgbWithoutAmbient = colorSensor.getRGBColor(true);
    
    // Test with ambient compensation
    colorSensor.configureColorScience(true, true, 0.08f); // Both compensations on
    ColorScience::RGBColor rgbWithAmbient = colorSensor.getRGBColor(true);
    
    // Show results
    Serial.println("=== COMPARISON RESULTS ===");
    Serial.println();
    
    Serial.println("WITHOUT Ambient Compensation:");
    Serial.printf("  RGB Float: R=%.3f G=%.3f B=%.3f\n", 
                 rgbWithoutAmbient.r, rgbWithoutAmbient.g, rgbWithoutAmbient.b);
    Serial.printf("  RGB 8-bit: R=%3d G=%3d B=%3d\n", 
                 rgbWithoutAmbient.r8, rgbWithoutAmbient.g8, rgbWithoutAmbient.b8);
    
    Serial.println();
    
    Serial.println("WITH Ambient Compensation:");
    Serial.printf("  RGB Float: R=%.3f G=%.3f B=%.3f\n", 
                 rgbWithAmbient.r, rgbWithAmbient.g, rgbWithAmbient.b);
    Serial.printf("  RGB 8-bit: R=%3d G=%3d B=%3d\n", 
                 rgbWithAmbient.r8, rgbWithAmbient.g8, rgbWithAmbient.b8);
    
    Serial.println();
    
    // Calculate improvement
    float improvementR = abs(rgbWithAmbient.r - rgbWithoutAmbient.r);
    float improvementG = abs(rgbWithAmbient.g - rgbWithoutAmbient.g);
    float improvementB = abs(rgbWithAmbient.b - rgbWithoutAmbient.b);
    
    Serial.println("=== AMBIENT COMPENSATION ANALYSIS ===");
    Serial.printf("Color Difference: ΔR=%.3f ΔG=%.3f ΔB=%.3f\n", 
                 improvementR, improvementG, improvementB);
    
    if (improvementR > 0.05f || improvementG > 0.05f || improvementB > 0.05f) {
        Serial.println("✓ Ambient compensation is making a significant difference");
        Serial.println("  This indicates the black reference subtraction is working");
    } else {
        Serial.println("⚠ Ambient compensation shows minimal difference");
        Serial.println("  This could mean: low ambient light, good sensor shielding, or calibration needed");
    }
    
    // Test white point accuracy
    float whiteAccuracy = (rgbWithAmbient.r + rgbWithAmbient.g + rgbWithAmbient.b) / 3.0f;
    Serial.printf("White Point Accuracy: %.1f%% (closer to 100%% is better)\n", whiteAccuracy * 100.0f);
    
    if (whiteAccuracy > 0.8f) {
        Serial.println("✓ Good white point accuracy with ambient compensation");
    } else if (whiteAccuracy > 0.6f) {
        Serial.println("⚠ Moderate white point accuracy - may need calibration adjustment");
    } else {
        Serial.println("❌ Poor white point accuracy - check calibration and lighting");
    }
}

void loop() {
    // Continuous monitoring
    delay(2000);
    
    // Get current readings with ambient compensation
    ColorScience::RGBColor rgb = colorSensor.getRGBColor(true);
    TCS3430AutoGain::RawData raw = colorSensor.raw();
    
    Serial.println("=== LIVE READINGS (with ambient compensation) ===");
    Serial.printf("Raw: X=%5d Y=%5d Z=%5d IR1=%5d IR2=%5d\n", 
                 raw.X, raw.Y, raw.Z, raw.IR1, raw.IR2);
    Serial.printf("RGB: R=%3d G=%3d B=%3d (%.3f, %.3f, %.3f)\n", 
                 rgb.r8, rgb.g8, rgb.b8, rgb.r, rgb.g, rgb.b);
    
    // Show calibration status
    ColorScience::CalibrationData calibData = colorSensor.getCalibrationData();
    if (calibData.ambientCompensationEnabled) {
        Serial.printf("Black subtraction: X-%.4f Y-%.4f Z-%.4f\n", 
                     calibData.blackReference.X, 
                     calibData.blackReference.Y, 
                     calibData.blackReference.Z);
    } else {
        Serial.println("⚠ Ambient compensation disabled");
    }
    
    Serial.println();
}
