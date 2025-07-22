/**
 * @file test_rgb_swatches.cpp
 * @brief Comprehensive RGB color swatch testing program
 * 
 * This program tests the color sensor accuracy using known RGB color swatches.
 * It compares sensor readings against reference values and calculates:
 * - Delta E color differences
 * - RGB accuracy percentages
 * - Overall calibration quality
 * 
 * Usage:
 * 1. Calibrate white and black references
 * 2. Place each color swatch over the sensor when prompted
 * 3. Review accuracy results and statistics
 */

#include <Arduino.h>
#include <Wire.h>
#include "lib/TCS3430AutoGain/TCS3430AutoGain.h"
#include "lib/SwatchTesting/SwatchTesting.h"

TCS3430AutoGain colorSensor;
SwatchTesting swatchTester;

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("=== RGB COLOR SWATCH TESTING SYSTEM ===");
    Serial.println("Professional color accuracy validation");
    Serial.println();
    
    // Initialize I2C and sensor
    Wire.begin(3, 4);
    
    if (!colorSensor.begin()) {
        Serial.println("ERROR: Sensor initialization failed!");
        while (1) delay(1000);
    }
    
    Serial.println("✓ Sensor initialized successfully");
    
    // Configure sensor for optimal accuracy
    colorSensor.power(true);
    colorSensor.mode(TCS3430AutoGain::ALS);
    colorSensor.gain(TCS3430AutoGain::X16);
    colorSensor.integrationTime(200.0f);  // Longer integration for accuracy
    
    // Configure advanced color science
    colorSensor.configureLEDIRCompensation(0.08f, 0.02f, true);
    colorSensor.setChannelIRLeakage(0.03f, 0.015f, 0.08f);
    colorSensor.configureColorScience(true, true, 0.08f);
    
    Serial.println("✓ Sensor configured for high accuracy");
    Serial.println();
    
    // Perform calibration
    performCalibration();
    
    // Run swatch tests
    runSwatchTests();
}

void performCalibration() {
    Serial.println("=== CALIBRATION PHASE ===");
    Serial.println();
    
    // Black reference calibration
    Serial.println("STEP 1: Black Reference Calibration");
    Serial.println("Please COVER the sensor completely with dark material");
    Serial.println("Press any key when ready...");
    waitForInput();
    
    Serial.println("Calibrating black reference...");
    if (colorSensor.calibrateBlackReference(15)) {
        Serial.println("✓ Black reference calibrated");
    } else {
        Serial.println("❌ Black reference calibration failed");
        return;
    }
    
    Serial.println();
    
    // White reference calibration
    Serial.println("STEP 2: White Reference Calibration");
    Serial.println("Please place a WHITE reference card over the sensor");
    Serial.println("Use a known white standard (e.g., white paper, white tile)");
    Serial.println("Press any key when ready...");
    waitForInput();
    
    Serial.println("Calibrating white reference...");
    if (colorSensor.calibrateWhiteReference(15)) {
        Serial.println("✓ White reference calibrated");
    } else {
        Serial.println("❌ White reference calibration failed");
        return;
    }
    
    Serial.println();
    Serial.println("✓ Calibration complete!");
    Serial.println();
}

void runSwatchTests() {
    Serial.println("=== SWATCH TESTING PHASE ===");
    Serial.println();
    
    // Choose test set
    Serial.println("Select test set:");
    Serial.println("1. Basic RGB colors (8 swatches)");
    Serial.println("2. Extended color set (16 swatches)");
    Serial.println("3. Pantone-like colors (12 swatches)");
    Serial.println("Enter choice (1-3): ");
    
    uint8_t choice = waitForChoice(1, 3);
    uint8_t swatchCount;
    const SwatchTesting::ColorSwatch* swatches = SwatchTesting::getStandardSwatches(choice - 1, swatchCount);
    
    if (!swatches) {
        Serial.println("Invalid choice!");
        return;
    }
    
    Serial.printf("Testing %d color swatches...\n", swatchCount);
    Serial.println();
    
    // Begin test session
    swatchTester.beginTestSession();
    
    // Test each swatch
    for (uint8_t i = 0; i < swatchCount; i++) {
        testSingleSwatch(swatches[i], i + 1, swatchCount);
    }
    
    // Finalize and show results
    SwatchTesting::TestSession session = swatchTester.finalizeTestSession();
    SwatchTesting::printTestSession(session);
    
    // Provide recommendations
    provideRecommendations(session);
}

void testSingleSwatch(const SwatchTesting::ColorSwatch& swatch, uint8_t current, uint8_t total) {
    Serial.printf("=== SWATCH %d/%d: %s ===\n", current, total, swatch.name);
    Serial.printf("Expected RGB: (%d, %d, %d)\n", 
                 swatch.referenceR, swatch.referenceG, swatch.referenceB);
    Serial.println("Please place this color swatch over the sensor");
    Serial.println("Press any key when ready...");
    waitForInput();
    
    // Take multiple readings for accuracy
    Serial.println("Taking readings...");
    ColorScience::RGBColor totalRGB = {0, 0, 0, 0, 0, 0};
    const int numReadings = 5;
    
    for (int i = 0; i < numReadings; i++) {
        ColorScience::RGBColor rgb = colorSensor.getRGBColor(true);
        totalRGB.r += rgb.r;
        totalRGB.g += rgb.g;
        totalRGB.b += rgb.b;
        totalRGB.r8 += rgb.r8;
        totalRGB.g8 += rgb.g8;
        totalRGB.b8 += rgb.b8;
        delay(100);
    }
    
    // Average the readings
    ColorScience::RGBColor avgRGB;
    avgRGB.r = totalRGB.r / numReadings;
    avgRGB.g = totalRGB.g / numReadings;
    avgRGB.b = totalRGB.b / numReadings;
    avgRGB.r8 = totalRGB.r8 / numReadings;
    avgRGB.g8 = totalRGB.g8 / numReadings;
    avgRGB.b8 = totalRGB.b8 / numReadings;
    
    // Test the swatch
    SwatchTesting::SwatchResult result = swatchTester.testSwatch(swatch, avgRGB);
    SwatchTesting::printSwatchResult(result);
    
    // Brief pause before next swatch
    delay(1000);
}

void provideRecommendations(const SwatchTesting::TestSession& session) {
    Serial.println("=== CALIBRATION RECOMMENDATIONS ===");
    Serial.println();
    
    if (session.sessionPassed) {
        Serial.println("🎉 EXCELLENT! Your color calibration is working well.");
        Serial.printf("Average accuracy: %.1f%% (Target: >80%%)\n", session.averageAccuracy);
        Serial.printf("Average Delta E: %.1f (Target: <6.0)\n", session.averageDeltaE);
    } else {
        Serial.println("⚠️  CALIBRATION NEEDS IMPROVEMENT");
        
        if (session.averageDeltaE > 10.0f) {
            Serial.println("• High color errors detected - check:");
            Serial.println("  - White/black reference calibration");
            Serial.println("  - LED lighting consistency");
            Serial.println("  - Sensor positioning and stability");
        }
        
        if (session.passedCount < session.numSwatches * 0.6f) {
            Serial.println("• Low pass rate - consider:");
            Serial.println("  - Recalibrating with better reference materials");
            Serial.println("  - Adjusting IR compensation settings");
            Serial.println("  - Using longer integration times");
        }
        
        if (session.worstDeltaE > 15.0f) {
            Serial.println("• Some colors show very poor accuracy:");
            Serial.println("  - Check for specific color bias (red, green, blue)");
            Serial.println("  - Verify sensor spectral response");
            Serial.println("  - Consider custom color matrix calibration");
        }
    }
    
    Serial.println();
    Serial.println("=== NEXT STEPS ===");
    
    if (session.averageAccuracy < 70.0f) {
        Serial.println("1. PRIORITY: Recalibrate white/black references");
        Serial.println("2. Adjust IR compensation parameters");
        Serial.println("3. Test with different lighting conditions");
    } else if (session.averageAccuracy < 85.0f) {
        Serial.println("1. Fine-tune IR compensation settings");
        Serial.println("2. Consider custom color matrix");
        Serial.println("3. Test with more color samples");
    } else {
        Serial.println("1. Your calibration is good for most applications");
        Serial.println("2. For critical color matching, consider:");
        Serial.println("   - Professional color standards");
        Serial.println("   - Spectrophotometer validation");
        Serial.println("   - Application-specific calibration");
    }
    
    Serial.println();
}

void waitForInput() {
    while (!Serial.available()) {
        delay(100);
    }
    while (Serial.available()) {
        Serial.read(); // Clear buffer
    }
}

uint8_t waitForChoice(uint8_t min, uint8_t max) {
    while (true) {
        if (Serial.available()) {
            uint8_t choice = Serial.read() - '0';
            while (Serial.available()) Serial.read(); // Clear buffer
            
            if (choice >= min && choice <= max) {
                return choice;
            }
            Serial.printf("Please enter a number between %d and %d: ", min, max);
        }
        delay(100);
    }
}

void loop() {
    // Continuous monitoring mode
    delay(5000);
    
    Serial.println("=== CONTINUOUS MONITORING ===");
    ColorScience::RGBColor rgb = colorSensor.getRGBColor(true);
    TCS3430AutoGain::RawData raw = colorSensor.raw();
    
    Serial.printf("Current RGB: (%3d, %3d, %3d)\n", rgb.r8, rgb.g8, rgb.b8);
    Serial.printf("Raw values: X=%d Y=%d Z=%d IR1=%d IR2=%d\n", 
                 raw.X, raw.Y, raw.Z, raw.IR1, raw.IR2);
    Serial.println("Place a color swatch to see live readings...");
    Serial.println();
}
