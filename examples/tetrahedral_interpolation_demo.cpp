/**
 * @file tetrahedral_interpolation_demo.cpp
 * @brief Comprehensive demonstration of tetrahedral interpolation for 4-point calibration
 * 
 * This example demonstrates:
 * - Tetrahedral interpolation setup and initialization
 * - Comparison between 2-point and 4-point conversion
 * - Performance benchmarking
 * - Accuracy validation
 * - Debug information and statistics
 */

#include "ColorConversionEnhanced.h"
#include "TetrahedralInterpolator.h"
#include "ColorScience.h"
#include "Arduino.h"

// Global calibration data for demonstration
ColorScience::CalibrationData demoCalibrationData;

// Initialize demo calibration data with realistic values
void initializeDemoCalibration() {
    Serial.println("=== Initializing Demo Calibration Data ===");
    
    // Simulate realistic TCS3430 sensor readings for each reference point
    
    // Black reference (LED off, black object)
    demoCalibrationData.blackReference.raw = {150.0f, 180.0f, 120.0f};  // Low values
    demoCalibrationData.blackReference.quality = 0.9f;
    demoCalibrationData.blackReference.timestamp = millis();
    demoCalibrationData.status.blackComplete = true;
    
    // White reference (LED on, white object)
    demoCalibrationData.whiteReference.raw = {45000.0f, 50000.0f, 35000.0f};  // High values
    demoCalibrationData.whiteReference.quality = 0.95f;
    demoCalibrationData.whiteReference.timestamp = millis();
    demoCalibrationData.status.whiteComplete = true;
    
    // Blue reference (LED on, blue object) - High Z channel
    demoCalibrationData.blueReference.raw = {8000.0f, 12000.0f, 35000.0f};  // Z dominant
    demoCalibrationData.blueReference.quality = 0.85f;
    demoCalibrationData.blueReference.timestamp = millis();
    demoCalibrationData.status.blueComplete = true;
    
    // Yellow reference (LED on, yellow object) - High X+Y channels
    demoCalibrationData.yellowReference.raw = {40000.0f, 45000.0f, 5000.0f};  // X+Y dominant
    demoCalibrationData.yellowReference.quality = 0.88f;
    demoCalibrationData.yellowReference.timestamp = millis();
    demoCalibrationData.status.yellowComplete = true;
    
    // Set calibration parameters
    demoCalibrationData.irCompensationFactor = 0.1f;
    demoCalibrationData.ambientCompensationEnabled = true;
    demoCalibrationData.interpolationMethod = ColorScience::CalibrationData::TETRAHEDRAL_4POINT;
    
    // Set validation thresholds
    demoCalibrationData.thresholds.blueZRatioMin = 0.6f;
    demoCalibrationData.thresholds.yellowXYRatioMin = 0.8f;
    demoCalibrationData.thresholds.maxCIEDE2000Error = 5.0f;
    
    Serial.println("Demo calibration data initialized:");
    Serial.println("  Black XYZ: " + String(demoCalibrationData.blackReference.raw.X) + ", " + 
                   String(demoCalibrationData.blackReference.raw.Y) + ", " + 
                   String(demoCalibrationData.blackReference.raw.Z));
    Serial.println("  White XYZ: " + String(demoCalibrationData.whiteReference.raw.X) + ", " + 
                   String(demoCalibrationData.whiteReference.raw.Y) + ", " + 
                   String(demoCalibrationData.whiteReference.raw.Z));
    Serial.println("  Blue XYZ: " + String(demoCalibrationData.blueReference.raw.X) + ", " + 
                   String(demoCalibrationData.blueReference.raw.Y) + ", " + 
                   String(demoCalibrationData.blueReference.raw.Z));
    Serial.println("  Yellow XYZ: " + String(demoCalibrationData.yellowReference.raw.X) + ", " + 
                   String(demoCalibrationData.yellowReference.raw.Y) + ", " + 
                   String(demoCalibrationData.yellowReference.raw.Z));
    Serial.println("  4-Point Calibrated: " + String(demoCalibrationData.status.is4PointCalibrated() ? "Yes" : "No"));
}

// Demonstrate tetrahedral interpolation weights
void demonstrateInterpolationWeights() {
    Serial.println("\n=== Tetrahedral Interpolation Weights Demo ===");
    
    TetrahedralInterpolator interpolator;
    if (!interpolator.initialize(demoCalibrationData)) {
        Serial.println("ERROR: Failed to initialize interpolator");
        return;
    }
    
    // Test various color points
    struct TestPoint {
        String name;
        uint16_t x, y, z;
    };
    
    TestPoint testPoints[] = {
        {"Near Black", 1000, 1200, 800},
        {"Near White", 40000, 45000, 30000},
        {"Near Blue", 5000, 8000, 25000},
        {"Near Yellow", 35000, 40000, 3000},
        {"Mid Gray", 20000, 22000, 18000},
        {"Purple-ish", 15000, 8000, 20000},
        {"Orange-ish", 30000, 20000, 5000}
    };
    
    for (const auto& point : testPoints) {
        TetrahedralWeights weights = interpolator.interpolate(point.x, point.y, point.z);
        
        Serial.println("Test Point: " + point.name + " XYZ(" + String(point.x) + "," + 
                      String(point.y) + "," + String(point.z) + ")");
        Serial.println("  Weights: " + weights.toString());
        Serial.println("  Valid: " + String(weights.isValid ? "Yes" : "No"));
        Serial.println("  Inside Tetrahedron: " + String(weights.isInsideTetrahedron() ? "Yes" : "No"));
        Serial.println();
    }
}

// Compare 2-point vs 4-point conversion results
void compareConversionMethods() {
    Serial.println("\n=== 2-Point vs 4-Point Conversion Comparison ===");
    
    // Initialize enhanced color converter
    if (!colorConverter.initialize(demoCalibrationData)) {
        Serial.println("ERROR: Failed to initialize color converter");
        return;
    }
    
    // Test various input colors
    struct TestColor {
        String name;
        uint16_t x, y, z, ir1, ir2;
    };
    
    TestColor testColors[] = {
        {"Neutral Gray", 20000, 22000, 18000, 500, 600},
        {"Reddish", 30000, 15000, 10000, 400, 500},
        {"Greenish", 15000, 35000, 12000, 450, 550},
        {"Bluish", 8000, 12000, 28000, 300, 400},
        {"Yellowish", 35000, 38000, 8000, 600, 700},
        {"Dark Color", 5000, 6000, 4000, 200, 250},
        {"Bright Color", 50000, 52000, 45000, 800, 900}
    };
    
    Serial.println("Color Name       | 2-Point RGB    | 4-Point RGB    | Method Used");
    Serial.println("-----------------|----------------|----------------|------------");
    
    for (const auto& color : testColors) {
        uint8_t r2, g2, b2;  // 2-point result
        uint8_t r4, g4, b4;  // 4-point result
        
        // Force 2-point conversion (simulate by temporarily disabling 4-point)
        ColorScience::CalibrationData temp2PointData = demoCalibrationData;
        temp2PointData.status.blueComplete = false;
        temp2PointData.status.yellowComplete = false;
        
        int method2 = colorConverter.convertXyZtoRgbEnhanced(color.x, color.y, color.z, 
                                                            color.ir1, color.ir2, 
                                                            r2, g2, b2, temp2PointData);
        
        // 4-point conversion
        int method4 = colorConverter.convertXyZtoRgbEnhanced(color.x, color.y, color.z, 
                                                            color.ir1, color.ir2, 
                                                            r4, g4, b4, demoCalibrationData);
        
        // Format output
        String colorName = color.name;
        while (colorName.length() < 16) colorName += " ";
        
        String rgb2Point = "(" + String(r2) + "," + String(g2) + "," + String(b2) + ")";
        while (rgb2Point.length() < 15) rgb2Point += " ";
        
        String rgb4Point = "(" + String(r4) + "," + String(g4) + "," + String(b4) + ")";
        while (rgb4Point.length() < 15) rgb4Point += " ";
        
        String methodUsed = (method4 == 2) ? "4-Point" : (method4 == 1) ? "2-Point" : "Fallback";
        
        Serial.println(colorName + " | " + rgb2Point + " | " + rgb4Point + " | " + methodUsed);
        
        // Calculate and show difference
        int deltaR = abs(r4 - r2);
        int deltaG = abs(g4 - g2);
        int deltaB = abs(b4 - b2);
        float totalDelta = sqrt(deltaR*deltaR + deltaG*deltaG + deltaB*deltaB);
        
        if (totalDelta > 10.0f) {
            Serial.println("                 | Significant difference: " + String(totalDelta, 1) + " units");
        }
    }
}

// Performance benchmark
void performanceBenchmark() {
    Serial.println("\n=== Performance Benchmark ===");
    
    const int ITERATIONS = 1000;
    
    // Benchmark enhanced conversion system
    String benchmarkResult = benchmarkConversionPerformance(ITERATIONS);
    Serial.println("Benchmark Results:");
    Serial.println(benchmarkResult);
    
    // Get detailed statistics
    uint32_t total2Point, total4Point, totalFallback;
    float accuracy4Point;
    colorConverter.getConversionStatistics(total2Point, total4Point, totalFallback, accuracy4Point);
    
    Serial.println("\nConversion Statistics:");
    Serial.println("  2-Point Conversions: " + String(total2Point));
    Serial.println("  4-Point Conversions: " + String(total4Point));
    Serial.println("  Fallback Conversions: " + String(totalFallback));
    Serial.println("  4-Point Accuracy: " + String(accuracy4Point, 1) + "%");
}

// Test accuracy validation
void testAccuracyValidation() {
    Serial.println("\n=== Accuracy Validation Test ===");
    
    String accuracyResult = testColorConversionAccuracy(demoCalibrationData);
    Serial.println("Accuracy Test Results:");
    Serial.println(accuracyResult);
    
    float accuracy = colorConverter.testConversionAccuracy(demoCalibrationData);
    if (accuracy >= 0) {
        Serial.println("Average Color Error: " + String(accuracy, 2) + " units");
        if (accuracy < 10.0f) {
            Serial.println("✓ Excellent accuracy");
        } else if (accuracy < 20.0f) {
            Serial.println("⚠ Good accuracy");
        } else {
            Serial.println("✗ Poor accuracy - calibration may need improvement");
        }
    } else {
        Serial.println("✗ Accuracy test failed");
    }
}

// Display debug information
void displayDebugInformation() {
    Serial.println("\n=== Debug Information ===");
    
    String debugInfo = colorConverter.getDebugInfo();
    Serial.println(debugInfo);
}

// Main setup function
void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("Tetrahedral Interpolation Demo");
    Serial.println("==============================");
    Serial.println("This demo showcases the enhanced 4-point color calibration");
    Serial.println("system with tetrahedral interpolation.\n");
    
    // Initialize demo calibration data
    initializeDemoCalibration();
    
    // Run demonstrations
    demonstrateInterpolationWeights();
    compareConversionMethods();
    performanceBenchmark();
    testAccuracyValidation();
    displayDebugInformation();
    
    Serial.println("\n=== Demo Complete ===");
    Serial.println("The tetrahedral interpolation system is ready for integration!");
}

void loop() {
    // Demo complete - just wait
    delay(10000);
}
