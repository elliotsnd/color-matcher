/**
 * @file enhanced_calibration_demo.cpp
 * @brief Demonstration of the enhanced 4-point calibration system
 * 
 * This example shows how to use the new unified CalibrationData structure
 * and implement the enhanced calibration workflow.
 */

#include "ColorScience.h"
#include "Arduino.h"

// Global calibration data using the new unified structure
ColorScience::CalibrationData calibrationData;

// Example: Initialize calibration data with default values
void initializeCalibrationData() {
    Serial.println("=== Initializing Enhanced Calibration System ===");
    
    // Set default thresholds
    calibrationData.thresholds.blueZRatioMin = 0.6f;
    calibrationData.thresholds.yellowXYRatioMin = 0.8f;
    calibrationData.thresholds.maxCIEDE2000Error = 5.0f;
    calibrationData.thresholds.minSignalNoiseRatio = 10.0f;
    calibrationData.thresholds.repeatabilityThreshold = 90.0f;
    
    // Set interpolation method
    calibrationData.interpolationMethod = ColorScience::CalibrationData::TETRAHEDRAL_4POINT;
    
    // Initialize lighting conditions
    calibrationData.lighting.calibrationBrightness = 20;
    calibrationData.lighting.brightnessLocked = false;
    calibrationData.lighting.ambientIRLevel = 0;
    
    // Initialize IR compensation
    calibrationData.irCompensationFactor = 0.1f;
    calibrationData.ambientCompensationEnabled = true;
    
    Serial.println("Calibration system initialized with default values");
    Serial.println("Ready for 4-point calibration sequence");
}

// Example: Simulate black reference calibration
bool calibrateBlackReference(uint16_t x, uint16_t y, uint16_t z, uint16_t ir1, uint16_t ir2) {
    Serial.println("=== Black Reference Calibration ===");
    
    // Store raw values
    calibrationData.blackReference.raw = {(float)x, (float)y, (float)z};
    calibrationData.blackReference.ir = {
        ir1 / 65535.0f, 
        ir2 / 65535.0f, 
        ir2 > 0 ? ir1 / (float)ir2 : 1.0f,
        (ir1 + ir2) / 2.0f / 65535.0f,
        0.0f, 0.0f
    };
    
    // Calculate normalized values
    calibrationData.blackReference.normalized = {
        x / 65535.0f,
        y / 65535.0f,
        z / 65535.0f
    };
    
    // Calculate quality metric (simplified)
    float totalSignal = x + y + z;
    calibrationData.blackReference.quality = totalSignal < 5000 ? 0.9f : 0.5f; // Lower signal = better black
    calibrationData.blackReference.timestamp = millis();
    
    // Mark as complete
    calibrationData.status.blackComplete = true;
    
    Serial.println("Black reference calibrated:");
    Serial.println("  Raw XYZ: " + String(x) + ", " + String(y) + ", " + String(z));
    Serial.println("  Quality: " + String(calibrationData.blackReference.quality));
    Serial.println("  Progress: " + String(calibrationData.status.progressPercent()) + "%");
    
    return true;
}

// Example: Simulate blue reference calibration with validation
bool calibrateBlueReference(uint16_t x, uint16_t y, uint16_t z, uint16_t ir1, uint16_t ir2) {
    Serial.println("=== Blue Reference Calibration ===");
    
    // Check prerequisites
    if (!calibrationData.status.is2PointCalibrated()) {
        Serial.println("ERROR: Must complete black and white calibration first");
        return false;
    }
    
    // Validate blue color quality
    float totalXYZ = x + y + z;
    float zRatio = z / totalXYZ;
    
    if (zRatio < calibrationData.thresholds.blueZRatioMin) {
        Serial.println("ERROR: Object is not blue enough");
        Serial.println("  Z ratio: " + String(zRatio) + " (required: " + 
                      String(calibrationData.thresholds.blueZRatioMin) + ")");
        Serial.println("  Use a more saturated blue object");
        return false;
    }
    
    // Store calibration data
    calibrationData.blueReference.raw = {(float)x, (float)y, (float)z};
    calibrationData.blueReference.ir = {
        ir1 / 65535.0f, 
        ir2 / 65535.0f, 
        ir2 > 0 ? ir1 / (float)ir2 : 1.0f,
        (ir1 + ir2) / 2.0f / 65535.0f,
        0.0f, 0.0f
    };
    
    calibrationData.blueReference.normalized = {
        x / 65535.0f,
        y / 65535.0f,
        z / 65535.0f
    };
    
    // Calculate quality based on Z dominance
    calibrationData.blueReference.quality = zRatio;
    calibrationData.blueReference.timestamp = millis();
    
    // Mark as complete
    calibrationData.status.blueComplete = true;
    
    Serial.println("Blue reference calibrated successfully:");
    Serial.println("  Raw XYZ: " + String(x) + ", " + String(y) + ", " + String(z));
    Serial.println("  Z ratio: " + String(zRatio));
    Serial.println("  Quality: " + String(calibrationData.blueReference.quality));
    Serial.println("  Progress: " + String(calibrationData.status.progressPercent()) + "%");
    
    return true;
}

// Example: Check calibration status
void printCalibrationStatus() {
    Serial.println("=== Calibration Status ===");
    Serial.println("Black: " + String(calibrationData.status.blackComplete ? "✓" : "✗"));
    Serial.println("White: " + String(calibrationData.status.whiteComplete ? "✓" : "✗"));
    Serial.println("Blue: " + String(calibrationData.status.blueComplete ? "✓" : "✗"));
    Serial.println("Yellow: " + String(calibrationData.status.yellowComplete ? "✓" : "✗"));
    Serial.println("2-Point Ready: " + String(calibrationData.status.is2PointCalibrated() ? "Yes" : "No"));
    Serial.println("4-Point Ready: " + String(calibrationData.status.is4PointCalibrated() ? "Yes" : "No"));
    Serial.println("Progress: " + String(calibrationData.status.progressPercent()) + "%");
    
    if (calibrationData.quality.lastValidationTime > 0) {
        Serial.println("Last Validation: " + String(calibrationData.quality.lastValidationTime));
        Serial.println("Accuracy: " + String(calibrationData.quality.overallAccuracy) + "%");
        Serial.println("Repeatability: " + String(calibrationData.quality.repeatability) + "%");
    }
}

// Example: Demonstrate the enhanced API response structure
String generateCalibrationStatusJSON() {
    String json = "{\n";
    json += "  \"status\": {\n";
    json += "    \"is2PointCalibrated\": " + String(calibrationData.status.is2PointCalibrated() ? "true" : "false") + ",\n";
    json += "    \"is4PointCalibrated\": " + String(calibrationData.status.is4PointCalibrated() ? "true" : "false") + ",\n";
    json += "    \"completedSteps\": " + String(calibrationData.status.completedSteps()) + ",\n";
    json += "    \"progressPercent\": " + String(calibrationData.status.progressPercent()) + "\n";
    json += "  },\n";
    json += "  \"references\": {\n";
    json += "    \"black\": {\n";
    json += "      \"complete\": " + String(calibrationData.status.blackComplete ? "true" : "false") + ",\n";
    if (calibrationData.status.blackComplete) {
        json += "      \"X\": " + String(calibrationData.blackReference.raw.X) + ",\n";
        json += "      \"Y\": " + String(calibrationData.blackReference.raw.Y) + ",\n";
        json += "      \"Z\": " + String(calibrationData.blackReference.raw.Z) + ",\n";
        json += "      \"quality\": " + String(calibrationData.blackReference.quality) + ",\n";
        json += "      \"timestamp\": " + String(calibrationData.blackReference.timestamp) + "\n";
    }
    json += "    }\n";
    json += "  },\n";
    json += "  \"thresholds\": {\n";
    json += "    \"blueZRatioMin\": " + String(calibrationData.thresholds.blueZRatioMin) + ",\n";
    json += "    \"yellowXYRatioMin\": " + String(calibrationData.thresholds.yellowXYRatioMin) + ",\n";
    json += "    \"maxCIEDE2000Error\": " + String(calibrationData.thresholds.maxCIEDE2000Error) + "\n";
    json += "  }\n";
    json += "}";
    return json;
}

// Example usage in setup()
void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("Enhanced 4-Point Calibration Demo");
    Serial.println("==================================");
    
    // Initialize the calibration system
    initializeCalibrationData();
    
    // Simulate calibration sequence
    Serial.println("\nSimulating calibration sequence...");
    
    // Step 1: Black calibration
    calibrateBlackReference(100, 120, 110, 50, 60);
    printCalibrationStatus();
    
    // Step 2: White calibration (simulated)
    calibrationData.status.whiteComplete = true; // For demo purposes
    
    // Step 3: Blue calibration
    calibrateBlueReference(2000, 3000, 8000, 200, 250); // High Z for blue
    printCalibrationStatus();
    
    // Generate JSON status
    Serial.println("\nJSON Status Response:");
    Serial.println(generateCalibrationStatusJSON());
}

void loop() {
    // Demo complete
    delay(10000);
}
