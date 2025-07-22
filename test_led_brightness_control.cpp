/**
 * @file test_led_brightness_control.cpp
 * @brief Test program for LED brightness optimization system
 * 
 * This program demonstrates and tests the automatic LED brightness control:
 * 1. Calibrates optimal LED brightness using white reference
 * 2. Tests automatic brightness adjustment with different objects
 * 3. Monitors signal levels and prevents saturation
 * 4. Shows statistics and performance metrics
 */

#include <Arduino.h>
#include <Wire.h>
#include "lib/TCS3430AutoGain/TCS3430AutoGain.h"
#include "lib/LEDBrightnessControl/LEDBrightnessControl.h"

// Hardware configuration
#define LED_PIN 5  // PWM pin for LED control
#define SDA_PIN 3  // I2C SDA pin
#define SCL_PIN 4  // I2C SCL pin

TCS3430AutoGain colorSensor;
LEDBrightnessControl ledControl;

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("=== LED BRIGHTNESS OPTIMIZATION TEST ===");
    Serial.println("Automatic LED control for optimal color sensing");
    Serial.println();
    
    // Initialize I2C and sensor
    Wire.begin(SDA_PIN, SCL_PIN);
    
    if (!colorSensor.begin()) {
        Serial.println("ERROR: Color sensor initialization failed!");
        while (1) delay(1000);
    }
    
    Serial.println("✓ Color sensor initialized");
    
    // Configure sensor for optimal performance
    colorSensor.power(true);
    colorSensor.mode(TCS3430AutoGain::ALS);
    colorSensor.gain(TCS3430AutoGain::X16);
    colorSensor.integrationTime(150.0f);
    
    // Configure advanced color science
    colorSensor.configureLEDIRCompensation(0.08f, 0.02f, true);
    colorSensor.configureColorScience(true, true, 0.08f);
    
    Serial.println("✓ Color sensor configured");
    
    // Initialize LED brightness control
    if (!ledControl.begin(LED_PIN, 128)) {
        Serial.println("ERROR: LED control initialization failed!");
        while (1) delay(1000);
    }
    
    Serial.println("✓ LED brightness control initialized");
    
    // Configure LED control parameters
    LEDBrightnessControl::LEDConfig config = ledControl.getConfig();
    config.targetMin = 45000;      // 70% of max (45,000/65,535)
    config.targetMax = 58000;      // 90% of max (58,000/65,535)
    config.adjustmentStep = 10;    // ±10 brightness units
    config.adjustmentDelay = 500;  // 500ms between adjustments
    config.stabilizationSamples = 3; // Average 3 samples
    ledControl.setConfig(config);
    
    Serial.println("✓ LED control configured");
    Serial.println();
    
    // Run calibration and tests
    runCalibrationTest();
    runOptimizationTest();
}

void runCalibrationTest() {
    Serial.println("=== LED BRIGHTNESS CALIBRATION TEST ===");
    Serial.println();
    
    Serial.println("STEP 1: White Reference Calibration");
    Serial.println("Please place a WHITE reference card over the sensor");
    Serial.println("This will determine the optimal LED brightness for accurate color sensing");
    Serial.println("Press any key when ready...");
    waitForInput();
    
    // Calibrate LED brightness for white reference
    bool calibSuccess = ledControl.calibrateWhiteReference(colorSensor, 52000); // Target 80% of max
    
    if (calibSuccess) {
        Serial.println("✓ LED brightness calibration successful!");
    } else {
        Serial.println("⚠ LED brightness calibration completed with warnings");
        Serial.println("  You may need to adjust the white reference or lighting conditions");
    }
    
    // Show calibration results
    ledControl.printStatus();
    
    // Test the calibrated brightness
    Serial.println("Testing calibrated brightness...");
    uint16_t maxChannel;
    bool optimal = ledControl.isSignalOptimal(colorSensor, maxChannel);
    
    Serial.printf("Max channel value: %d\n", maxChannel);
    Serial.printf("Signal level: %.1f%% of full scale\n", (float)maxChannel / 65535.0f * 100.0f);
    Serial.printf("Signal quality: %s\n", optimal ? "OPTIMAL ✓" : "NEEDS ADJUSTMENT ⚠");
    Serial.println();
}

void runOptimizationTest() {
    Serial.println("=== AUTOMATIC BRIGHTNESS OPTIMIZATION TEST ===");
    Serial.println();
    
    Serial.println("This test will demonstrate automatic LED brightness adjustment");
    Serial.println("Try placing different colored objects over the sensor:");
    Serial.println("- Dark objects (should increase LED brightness)");
    Serial.println("- Bright/reflective objects (should decrease LED brightness)");
    Serial.println("- Various colors to see dynamic adjustment");
    Serial.println();
    Serial.println("Press any key to start continuous monitoring...");
    waitForInput();
    
    // Enable auto-adjustment
    ledControl.enableAutoAdjustment(true);
    
    Serial.println("=== CONTINUOUS MONITORING ACTIVE ===");
    Serial.println("LED brightness will automatically adjust to maintain optimal signal levels");
    Serial.println("Target range: 45,000 - 58,000 counts (70-90% of full scale)");
    Serial.println();
    
    unsigned long lastPrint = 0;
    unsigned long lastStats = 0;
    
    while (true) {
        // Perform brightness optimization
        LEDBrightnessControl::AdjustmentResult result = ledControl.optimizeBrightness(colorSensor);
        
        // Print status every 2 seconds
        if (millis() - lastPrint > 2000) {
            printCurrentStatus(result);
            lastPrint = millis();
        }
        
        // Print statistics every 30 seconds
        if (millis() - lastStats > 30000) {
            printStatistics();
            lastStats = millis();
        }
        
        delay(100);  // Main loop delay
    }
}

void printCurrentStatus(LEDBrightnessControl::AdjustmentResult result) {
    // Get current readings
    TCS3430AutoGain::RawData raw = colorSensor.raw();
    ColorScience::RGBColor rgb = colorSensor.getRGBColor(true);
    uint16_t maxChannel = max(max(raw.X, raw.Y), raw.Z);
    
    // Calculate signal level percentage
    float signalPercent = (float)maxChannel / 65535.0f * 100.0f;
    
    // Determine signal quality
    const char* quality;
    if (maxChannel > 58000) {
        quality = "HIGH (risk of saturation)";
    } else if (maxChannel > 45000) {
        quality = "OPTIMAL";
    } else if (maxChannel > 10000) {
        quality = "LOW (but usable)";
    } else {
        quality = "VERY LOW (noisy)";
    }
    
    Serial.println("=== CURRENT STATUS ===");
    Serial.printf("LED Brightness: %3d/255 (%2.0f%%)\n", 
                 ledControl.getBrightness(), 
                 ledControl.getBrightness() / 255.0f * 100.0f);
    Serial.printf("Max Channel: %5d (%4.1f%% of full scale)\n", maxChannel, signalPercent);
    Serial.printf("Signal Quality: %s\n", quality);
    Serial.printf("Raw XYZ: X=%5d Y=%5d Z=%5d\n", raw.X, raw.Y, raw.Z);
    Serial.printf("RGB Output: R=%3d G=%3d B=%3d\n", rgb.r8, rgb.g8, rgb.b8);
    Serial.printf("Last Action: %s\n", LEDBrightnessControl::adjustmentResultToString(result));
    Serial.println();
}

void printStatistics() {
    Serial.println("=== BRIGHTNESS CONTROL STATISTICS ===");
    
    uint32_t totalAdjustments, increasedCount, decreasedCount;
    float avgMaxChannel;
    ledControl.getStatistics(totalAdjustments, increasedCount, decreasedCount, avgMaxChannel);
    
    Serial.printf("Total Adjustments: %lu\n", totalAdjustments);
    Serial.printf("Brightness Increased: %lu times\n", increasedCount);
    Serial.printf("Brightness Decreased: %lu times\n", decreasedCount);
    Serial.printf("Average Max Channel: %.0f counts (%.1f%% of full scale)\n", 
                 avgMaxChannel, avgMaxChannel / 65535.0f * 100.0f);
    
    if (totalAdjustments > 0) {
        float increaseRate = (float)increasedCount / totalAdjustments * 100.0f;
        float decreaseRate = (float)decreasedCount / totalAdjustments * 100.0f;
        Serial.printf("Adjustment Pattern: %.1f%% increases, %.1f%% decreases\n", 
                     increaseRate, decreaseRate);
    }
    
    // Provide performance assessment
    if (avgMaxChannel >= 45000 && avgMaxChannel <= 58000) {
        Serial.println("✓ Performance: EXCELLENT - Signal levels consistently optimal");
    } else if (avgMaxChannel >= 35000 && avgMaxChannel <= 65000) {
        Serial.println("⚠ Performance: GOOD - Signal levels mostly acceptable");
    } else {
        Serial.println("❌ Performance: POOR - Signal levels frequently suboptimal");
        if (avgMaxChannel < 35000) {
            Serial.println("  Recommendation: Increase LED power or check object reflectivity");
        } else {
            Serial.println("  Recommendation: Decrease LED power or check for saturation");
        }
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

void loop() {
    // Main loop is handled in runOptimizationTest()
    // This function won't be reached in normal operation
    delay(1000);
}
