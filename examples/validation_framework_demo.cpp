/**
 * @file validation_framework_demo.cpp
 * @brief Comprehensive demonstration of the validation framework
 * 
 * This example demonstrates:
 * - CIEDE2000 color difference calculations
 * - Validation test suite with standardized test colors
 * - Professional quality metrics and reporting
 * - Color space conversions (RGB, LAB, LCH, XYZ)
 * - Batch validation testing
 * - Quality assessment and recommendations
 */

#include "ValidationTestSuite.h"
#include "CIEDE2000.h"
#include "ColorConversionEnhanced.h"
#include "Arduino.h"

// Global instances for demonstration
CIEDE2000 colorEngine;
ValidationTestSuite testSuite;
ColorConversionEnhanced demoConverter;

// Simulated calibration data for demonstration
ColorScience::CalibrationData demoCalibData;

// Initialize demo calibration data
void initializeDemoCalibration() {
    Serial.println("=== Initializing Demo Calibration Data ===");
    
    // Set up realistic calibration data
    demoCalibData.blackReference.raw = {150.0f, 180.0f, 120.0f};
    demoCalibData.blackReference.quality = 0.9f;
    demoCalibData.status.blackComplete = true;
    
    demoCalibData.whiteReference.raw = {45000.0f, 50000.0f, 35000.0f};
    demoCalibData.whiteReference.quality = 0.95f;
    demoCalibData.status.whiteComplete = true;
    
    demoCalibData.blueReference.raw = {8000.0f, 12000.0f, 35000.0f};
    demoCalibData.blueReference.quality = 0.85f;
    demoCalibData.status.blueComplete = true;
    
    demoCalibData.yellowReference.raw = {40000.0f, 45000.0f, 5000.0f};
    demoCalibData.yellowReference.quality = 0.88f;
    demoCalibData.status.yellowComplete = true;
    
    // Initialize color converter
    demoConverter.initialize(demoCalibData);
    
    Serial.println("Demo calibration data initialized");
    Serial.println("4-Point Calibrated: " + String(demoCalibData.status.is4PointCalibrated() ? "Yes" : "No"));
}

// Demonstrate CIEDE2000 color difference calculations
void demonstrateCIEDE2000() {
    Serial.println("\n=== CIEDE2000 Color Difference Demonstration ===");
    
    // Test color pairs with known differences
    struct ColorPair {
        String name;
        RGBColor color1;
        RGBColor color2;
        String expectedQuality;
    };
    
    ColorPair testPairs[] = {
        {"Nearly Identical", RGBColor(128, 128, 128), RGBColor(130, 127, 129), "Excellent"},
        {"Slight Difference", RGBColor(255, 0, 0), RGBColor(250, 10, 5), "Good"},
        {"Moderate Difference", RGBColor(0, 255, 0), RGBColor(50, 200, 30), "Acceptable"},
        {"Large Difference", RGBColor(0, 0, 255), RGBColor(255, 255, 0), "Poor"},
        {"Extreme Difference", RGBColor(0, 0, 0), RGBColor(255, 255, 255), "Unacceptable"}
    };
    
    for (const auto& pair : testPairs) {
        // Convert to LAB
        LABColor lab1 = colorEngine.rgbToLAB(pair.color1);
        LABColor lab2 = colorEngine.rgbToLAB(pair.color2);
        
        // Calculate color difference
        ColorDifferenceResult result = colorEngine.calculateDeltaE2000(lab1, lab2);
        
        Serial.println("\nColor Pair: " + pair.name);
        Serial.println("  Color 1: " + pair.color1.toString() + " -> " + lab1.toString());
        Serial.println("  Color 2: " + pair.color2.toString() + " -> " + lab2.toString());
        Serial.println("  CIEDE2000 ΔE: " + String(result.deltaE2000, 2));
        Serial.println("  CIE76 ΔE*ab: " + String(result.deltaE76, 2));
        Serial.println("  Quality Level: " + result.qualityLevel);
        Serial.println("  Acceptable: " + String(result.acceptable ? "Yes" : "No"));
        Serial.println("  Expected: " + pair.expectedQuality);
        
        // Show component differences
        Serial.println("  Component Differences:");
        Serial.println("    ΔL*: " + String(result.deltaL, 2) + " (Lightness)");
        Serial.println("    ΔC*: " + String(result.deltaC, 2) + " (Chroma)");
        Serial.println("    ΔH*: " + String(result.deltaH, 2) + " (Hue)");
    }
}

// Demonstrate color space conversions
void demonstrateColorSpaceConversions() {
    Serial.println("\n=== Color Space Conversion Demonstration ===");
    
    // Test colors for conversion
    RGBColor testColors[] = {
        RGBColor(255, 0, 0),    // Red
        RGBColor(0, 255, 0),    // Green
        RGBColor(0, 0, 255),    // Blue
        RGBColor(128, 128, 128), // Gray
        RGBColor(255, 255, 0),  // Yellow
        RGBColor(255, 0, 255)   // Magenta
    };
    
    String colorNames[] = {"Red", "Green", "Blue", "Gray", "Yellow", "Magenta"};
    
    for (int i = 0; i < 6; i++) {
        RGBColor rgb = testColors[i];
        XYZColor xyz = colorEngine.rgbToXYZ(rgb);
        LABColor lab = colorEngine.xyzToLAB(xyz);
        LCHColor lch = colorEngine.labToLCH(lab);
        
        Serial.println("\nColor: " + colorNames[i]);
        Serial.println("  RGB: " + rgb.toString());
        Serial.println("  XYZ: " + xyz.toString());
        Serial.println("  LAB: " + lab.toString());
        Serial.println("  LCH: " + lch.toString());
        
        // Validate LAB values
        if (colorEngine.validateLABColor(lab)) {
            Serial.println("  LAB values: Valid");
        } else {
            Serial.println("  LAB values: Invalid");
        }
    }
}

// Demonstrate validation test suites
void demonstrateValidationTestSuites() {
    Serial.println("\n=== Validation Test Suite Demonstration ===");
    
    // Initialize test suite
    if (!testSuite.initialize(&demoConverter, &demoCalibData)) {
        Serial.println("ERROR: Failed to initialize test suite");
        return;
    }
    
    // Test different test suites
    String suiteNames[] = {"basic", "grayscale", "saturated"};
    
    for (const auto& suiteName : suiteNames) {
        Serial.println("\n--- Testing Suite: " + suiteName + " ---");
        
        if (testSuite.loadTestSuite(suiteName)) {
            Serial.println("Test suite loaded: " + String(testSuite.getTestColorCount()) + " colors");
            
            // Perform quick validation
            ValidationResults results = testSuite.performQuickValidation(5);
            
            Serial.println("Validation Results:");
            Serial.println("  Total Tests: " + String(results.totalTests));
            Serial.println("  Passed: " + String(results.passedTests));
            Serial.println("  Failed: " + String(results.failedTests));
            Serial.println("  Pass Rate: " + String(results.getPassRate(), 1) + "%");
            Serial.println("  Average ΔE: " + String(results.averageDeltaE, 2));
            Serial.println("  Max ΔE: " + String(results.maxDeltaE, 2));
            Serial.println("  Min ΔE: " + String(results.minDeltaE, 2));
            Serial.println("  Quality Level: " + results.qualityLevel);
            Serial.println("  Calibration Valid: " + String(results.calibrationValid ? "Yes" : "No"));
            Serial.println("  Processing Time: " + String(results.totalProcessingTime) + " μs");
        }
    }
}

// Demonstrate individual test results
void demonstrateIndividualTestResults() {
    Serial.println("\n=== Individual Test Results Demonstration ===");
    
    // Load basic test suite
    testSuite.loadTestSuite("basic");
    
    // Perform validation
    ValidationResults results = testSuite.performValidation();
    
    // Get individual test results
    TestResult individualResults[20];
    int resultCount = testSuite.getIndividualTestResults(individualResults, 20);
    
    Serial.println("Individual Test Results (" + String(resultCount) + " tests):");
    Serial.println("Color Name           | Expected RGB    | Measured RGB    | ΔE     | Pass | Accuracy");
    Serial.println("---------------------|-----------------|-----------------|--------|------|----------");
    
    for (int i = 0; i < resultCount; i++) {
        const TestResult& test = individualResults[i];
        
        String colorName = test.testColor.name;
        while (colorName.length() < 20) colorName += " ";
        
        String expectedRGB = test.testColor.expectedRGB.toString();
        while (expectedRGB.length() < 15) expectedRGB += " ";
        
        String measuredRGB = test.measuredRGB.toString();
        while (measuredRGB.length() < 15) measuredRGB += " ";
        
        String deltaE = String(test.colorDiff.deltaE2000, 2);
        while (deltaE.length() < 6) deltaE += " ";
        
        String pass = test.passed ? "PASS" : "FAIL";
        while (pass.length() < 4) pass += " ";
        
        String accuracy = String(test.accuracy, 1) + "%";
        
        Serial.println(colorName + " | " + expectedRGB + " | " + measuredRGB + " | " + 
                      deltaE + " | " + pass + " | " + accuracy);
    }
}

// Demonstrate quality assessment and recommendations
void demonstrateQualityAssessment() {
    Serial.println("\n=== Quality Assessment and Recommendations ===");
    
    // Perform comprehensive validation
    testSuite.loadTestSuite("comprehensive");
    ValidationResults results = testSuite.performValidation();
    
    // Generate detailed report
    String report = testSuite.generateValidationReport(false);
    Serial.println("Validation Report:");
    Serial.println(report);
    
    // Get recommendations
    String recommendations = testSuite.getValidationRecommendations();
    Serial.println("\nRecommendations:");
    Serial.println(recommendations);
    
    // Generate quality assessment
    String qualityAssessment = generateQualityAssessment(results);
    Serial.println("\nQuality Assessment:");
    Serial.println(qualityAssessment);
}

// Demonstrate batch color difference calculations
void demonstrateBatchCalculations() {
    Serial.println("\n=== Batch Color Difference Calculations ===");
    
    // Create arrays of reference and measured colors
    LABColor referenceColors[5] = {
        LABColor(50.0f, 0.0f, 0.0f),    // Neutral gray
        LABColor(30.0f, 40.0f, 20.0f),  // Reddish
        LABColor(70.0f, -30.0f, 10.0f), // Greenish
        LABColor(40.0f, 10.0f, -40.0f), // Bluish
        LABColor(80.0f, 20.0f, 60.0f)   // Yellowish
    };
    
    LABColor measuredColors[5] = {
        LABColor(52.0f, 2.0f, -1.0f),   // Slightly off gray
        LABColor(28.0f, 42.0f, 18.0f),  // Slightly off red
        LABColor(72.0f, -28.0f, 12.0f), // Slightly off green
        LABColor(38.0f, 12.0f, -38.0f), // Slightly off blue
        LABColor(82.0f, 18.0f, 62.0f)   // Slightly off yellow
    };
    
    ColorDifferenceResult results[5];
    
    // Calculate batch differences
    float averageDeltaE = colorEngine.calculateBatchDifferences(referenceColors, measuredColors, 5, results);
    
    Serial.println("Batch Color Difference Results:");
    Serial.println("Average ΔE: " + String(averageDeltaE, 2));
    Serial.println();
    
    String colorNames[] = {"Neutral Gray", "Reddish", "Greenish", "Bluish", "Yellowish"};
    
    for (int i = 0; i < 5; i++) {
        Serial.println("Color " + String(i + 1) + " (" + colorNames[i] + "):");
        Serial.println("  Reference: " + referenceColors[i].toString());
        Serial.println("  Measured:  " + measuredColors[i].toString());
        Serial.println("  ΔE2000: " + String(results[i].deltaE2000, 2));
        Serial.println("  Quality: " + results[i].qualityLevel);
        Serial.println("  Acceptable: " + String(results[i].acceptable ? "Yes" : "No"));
        Serial.println();
    }
}

// Demonstrate validation configuration
void demonstrateValidationConfiguration() {
    Serial.println("\n=== Validation Configuration Demonstration ===");
    
    // Test different configurations
    ValidationConfig configs[] = {
        {"basic", 2.0f, false, true, true, "sRGB", "critical"},
        {"basic", 3.0f, true, true, true, "sRGB", "general"},
        {"basic", 5.0f, false, false, false, "AdobeRGB", "industrial"}
    };
    
    String configNames[] = {"Critical Application", "General Application", "Industrial Application"};
    
    for (int i = 0; i < 3; i++) {
        Serial.println("\n--- Configuration: " + configNames[i] + " ---");
        
        testSuite.setValidationConfig(configs[i]);
        testSuite.loadTestSuite("basic");
        
        ValidationResults results = testSuite.performQuickValidation(3);
        
        Serial.println("Configuration Settings:");
        Serial.println("  Global Tolerance: " + String(configs[i].globalTolerance));
        Serial.println("  Strict Tolerance: " + String(configs[i].useStrictTolerance ? "Yes" : "No"));
        Serial.println("  Color Space: " + configs[i].colorSpace);
        Serial.println("  Application: " + configs[i].application);
        
        Serial.println("Results:");
        Serial.println("  Pass Rate: " + String(results.getPassRate(), 1) + "%");
        Serial.println("  Average ΔE: " + String(results.averageDeltaE, 2));
        Serial.println("  Quality Level: " + results.qualityLevel);
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("Validation Framework Demo");
    Serial.println("=========================");
    Serial.println("This demo showcases the comprehensive validation framework");
    Serial.println("with CIEDE2000 color difference calculations and professional");
    Serial.println("quality metrics for color calibration systems.\n");
    
    // Initialize demo calibration
    initializeDemoCalibration();
    
    // Run demonstrations
    demonstrateCIEDE2000();
    demonstrateColorSpaceConversions();
    demonstrateValidationTestSuites();
    demonstrateIndividualTestResults();
    demonstrateQualityAssessment();
    demonstrateBatchCalculations();
    demonstrateValidationConfiguration();
    
    Serial.println("\n=== Demo Complete ===");
    Serial.println("The validation framework provides:");
    Serial.println("✓ CIEDE2000 color difference calculations");
    Serial.println("✓ Professional color space conversions");
    Serial.println("✓ Standardized test color suites");
    Serial.println("✓ Comprehensive quality metrics");
    Serial.println("✓ Batch validation testing");
    Serial.println("✓ Detailed reporting and recommendations");
    Serial.println("✓ Configurable validation parameters");
    Serial.println("✓ Industry-standard quality assessment");
    Serial.println("\nThe validation framework is ready for professional color calibration testing!");
}

void loop() {
    // Demo complete - just wait
    delay(10000);
}
