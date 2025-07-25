/**
 * @file ValidationTestSuite.cpp
 * @brief Implementation of comprehensive validation test suite
 */

#include "ValidationTestSuite.h"

// Global instance
ValidationTestSuite validationSuite;

// Constructor
ValidationTestSuite::ValidationTestSuite() 
    : colorConverter(nullptr), calibrationData(nullptr), testColorCount(0) {
    // Initialize with default configuration
    config.testSuite = "basic";
    config.globalTolerance = 3.0f;
    config.useStrictTolerance = false;
    config.includePerformance = true;
    config.generateReport = true;
    config.colorSpace = "sRGB";
    config.application = "general";
}

// Initialize validation test suite
bool ValidationTestSuite::initialize(ColorConversionEnhanced* converter, ColorScience::CalibrationData* calibData) {
    colorConverter = converter;
    calibrationData = calibData;
    
    if (!colorConverter || !calibrationData) {
        Serial.println("ERROR: Missing dependencies for validation test suite");
        return false;
    }
    
    Serial.println("Validation test suite initialized successfully");
    
    // Load default test suite
    loadTestSuite("basic");
    
    return true;
}

// Initialize basic RGB test colors
void ValidationTestSuite::initializeBasicTestColors() {
    testColorCount = 0;
    
    // Primary colors
    testColors[testColorCount++] = TestColor("Red", RGBColor(255, 0, 0), 45000, 15000, 5000, 2.0f, "primary");
    testColors[testColorCount++] = TestColor("Green", RGBColor(0, 255, 0), 15000, 50000, 8000, 2.0f, "primary");
    testColors[testColorCount++] = TestColor("Blue", RGBColor(0, 0, 255), 8000, 12000, 35000, 2.0f, "primary");
    
    // Secondary colors
    testColors[testColorCount++] = TestColor("Cyan", RGBColor(0, 255, 255), 23000, 62000, 43000, 3.0f, "secondary");
    testColors[testColorCount++] = TestColor("Magenta", RGBColor(255, 0, 255), 53000, 27000, 40000, 3.0f, "secondary");
    testColors[testColorCount++] = TestColor("Yellow", RGBColor(255, 255, 0), 60000, 65000, 13000, 2.5f, "secondary");
    
    // Grayscale
    testColors[testColorCount++] = TestColor("Black", RGBColor(0, 0, 0), 150, 180, 120, 1.0f, "grayscale");
    testColors[testColorCount++] = TestColor("Gray 50%", RGBColor(128, 128, 128), 20000, 22000, 18000, 2.0f, "grayscale");
    testColors[testColorCount++] = TestColor("White", RGBColor(255, 255, 255), 45000, 50000, 35000, 1.5f, "grayscale");
    
    Serial.println("Basic test colors initialized: " + String(testColorCount) + " colors");
}

// Initialize Macbeth ColorChecker simulation
void ValidationTestSuite::initializeMacbethColorChecker() {
    testColorCount = 0;
    
    // Simplified Macbeth ColorChecker patches (first 12 patches)
    testColors[testColorCount++] = TestColor("Dark Skin", RGBColor(115, 82, 68), 8500, 7200, 5800, 3.0f, "macbeth");
    testColors[testColorCount++] = TestColor("Light Skin", RGBColor(194, 150, 130), 25000, 23000, 19000, 3.0f, "macbeth");
    testColors[testColorCount++] = TestColor("Blue Sky", RGBColor(98, 122, 157), 12000, 15000, 28000, 3.5f, "macbeth");
    testColors[testColorCount++] = TestColor("Foliage", RGBColor(87, 108, 67), 9000, 12000, 7500, 3.0f, "macbeth");
    testColors[testColorCount++] = TestColor("Blue Flower", RGBColor(133, 128, 177), 16000, 17000, 32000, 3.5f, "macbeth");
    testColors[testColorCount++] = TestColor("Bluish Green", RGBColor(103, 189, 170), 18000, 35000, 28000, 3.0f, "macbeth");
    testColors[testColorCount++] = TestColor("Orange", RGBColor(214, 126, 44), 28000, 18000, 6000, 3.0f, "macbeth");
    testColors[testColorCount++] = TestColor("Purplish Blue", RGBColor(80, 91, 166), 8500, 10000, 30000, 4.0f, "macbeth");
    testColors[testColorCount++] = TestColor("Moderate Red", RGBColor(193, 90, 99), 25000, 12000, 14000, 3.0f, "macbeth");
    testColors[testColorCount++] = TestColor("Purple", RGBColor(94, 60, 108), 9000, 6500, 15000, 4.0f, "macbeth");
    testColors[testColorCount++] = TestColor("Yellow Green", RGBColor(157, 188, 64), 28000, 38000, 8000, 3.0f, "macbeth");
    testColors[testColorCount++] = TestColor("Orange Yellow", RGBColor(224, 163, 46), 35000, 28000, 6500, 3.0f, "macbeth");
    
    Serial.println("Macbeth ColorChecker colors initialized: " + String(testColorCount) + " colors");
}

// Initialize grayscale test colors
void ValidationTestSuite::initializeGrayscaleColors() {
    testColorCount = 0;
    
    // 11-step grayscale from 0% to 100%
    for (int i = 0; i <= 10; i++) {
        uint8_t grayLevel = (i * 255) / 10;
        uint16_t sensorValue = 150 + (i * 4985); // Linear interpolation from black to white sensor values
        
        String name = "Gray " + String(i * 10) + "%";
        testColors[testColorCount++] = TestColor(name, RGBColor(grayLevel, grayLevel, grayLevel),
                                               sensorValue, sensorValue + 500, sensorValue - 300,
                                               1.5f, "grayscale");
    }
    
    Serial.println("Grayscale test colors initialized: " + String(testColorCount) + " colors");
}

// Initialize saturated test colors
void ValidationTestSuite::initializeSaturatedColors() {
    testColorCount = 0;
    
    // Highly saturated colors for gamut testing
    testColors[testColorCount++] = TestColor("Pure Red", RGBColor(255, 0, 0), 45000, 15000, 5000, 2.0f, "saturated");
    testColors[testColorCount++] = TestColor("Pure Green", RGBColor(0, 255, 0), 15000, 50000, 8000, 2.0f, "saturated");
    testColors[testColorCount++] = TestColor("Pure Blue", RGBColor(0, 0, 255), 8000, 12000, 35000, 2.0f, "saturated");
    testColors[testColorCount++] = TestColor("Pure Cyan", RGBColor(0, 255, 255), 23000, 62000, 43000, 3.0f, "saturated");
    testColors[testColorCount++] = TestColor("Pure Magenta", RGBColor(255, 0, 255), 53000, 27000, 40000, 3.0f, "saturated");
    testColors[testColorCount++] = TestColor("Pure Yellow", RGBColor(255, 255, 0), 60000, 65000, 13000, 2.5f, "saturated");
    
    // Additional saturated colors
    testColors[testColorCount++] = TestColor("Deep Orange", RGBColor(255, 69, 0), 42000, 12000, 3000, 3.5f, "saturated");
    testColors[testColorCount++] = TestColor("Deep Purple", RGBColor(148, 0, 211), 25000, 8000, 35000, 4.0f, "saturated");
    testColors[testColorCount++] = TestColor("Lime Green", RGBColor(50, 205, 50), 12000, 40000, 10000, 3.0f, "saturated");
    testColors[testColorCount++] = TestColor("Hot Pink", RGBColor(255, 20, 147), 48000, 15000, 25000, 3.5f, "saturated");
    
    Serial.println("Saturated test colors initialized: " + String(testColorCount) + " colors");
}

// Load predefined test suite
bool ValidationTestSuite::loadTestSuite(const String& suiteName) {
    config.testSuite = suiteName;
    
    if (suiteName == "basic") {
        initializeBasicTestColors();
    } else if (suiteName == "macbeth") {
        initializeMacbethColorChecker();
    } else if (suiteName == "grayscale") {
        initializeGrayscaleColors();
    } else if (suiteName == "saturated") {
        initializeSaturatedColors();
    } else if (suiteName == "comprehensive") {
        // Load all test colors
        initializeBasicTestColors();
        int basicCount = testColorCount;
        
        // Add some Macbeth colors
        testColors[testColorCount++] = TestColor("Dark Skin", RGBColor(115, 82, 68), 8500, 7200, 5800, 3.0f, "macbeth");
        testColors[testColorCount++] = TestColor("Light Skin", RGBColor(194, 150, 130), 25000, 23000, 19000, 3.0f, "macbeth");
        testColors[testColorCount++] = TestColor("Blue Sky", RGBColor(98, 122, 157), 12000, 15000, 28000, 3.5f, "macbeth");
        
        // Add some saturated colors
        testColors[testColorCount++] = TestColor("Deep Orange", RGBColor(255, 69, 0), 42000, 12000, 3000, 3.5f, "saturated");
        testColors[testColorCount++] = TestColor("Deep Purple", RGBColor(148, 0, 211), 25000, 8000, 35000, 4.0f, "saturated");
        
        Serial.println("Comprehensive test suite loaded: " + String(testColorCount) + " colors");
    } else {
        Serial.println("ERROR: Unknown test suite: " + suiteName);
        return false;
    }
    
    // Convert RGB to LAB for all test colors
    for (int i = 0; i < testColorCount; i++) {
        testColors[i].expectedLAB = colorDifferenceEngine.rgbToLAB(testColors[i].expectedRGB, config.colorSpace);
    }
    
    Serial.println("Test suite '" + suiteName + "' loaded successfully");
    return true;
}

// Add custom test color
bool ValidationTestSuite::addTestColor(const TestColor& testColor) {
    if (testColorCount >= MAX_TEST_COLORS) {
        Serial.println("ERROR: Maximum test colors reached");
        return false;
    }
    
    testColors[testColorCount] = testColor;
    testColors[testColorCount].expectedLAB = colorDifferenceEngine.rgbToLAB(testColor.expectedRGB, config.colorSpace);
    testColorCount++;
    
    return true;
}

// Clear all test colors
void ValidationTestSuite::clearTestColors() {
    testColorCount = 0;
    Serial.println("All test colors cleared");
}

// Set validation configuration
void ValidationTestSuite::setValidationConfig(const ValidationConfig& validationConfig) {
    config = validationConfig;
    Serial.println("Validation configuration updated:");
    Serial.println("  Test Suite: " + config.testSuite);
    Serial.println("  Global Tolerance: " + String(config.globalTolerance));
    Serial.println("  Color Space: " + config.colorSpace);
    Serial.println("  Application: " + config.application);
}

// Perform single color test
TestResult ValidationTestSuite::performSingleColorTest(const TestColor& testColor) {
    TestResult result;
    result.testColor = testColor;
    
    uint32_t startTime = micros();
    
    // Perform color conversion
    uint8_t measuredR, measuredG, measuredB;
    uint16_t ir1 = 500, ir2 = 600; // Simulated IR values
    
    int conversionMethod = colorConverter->convertXyZtoRgbEnhanced(
        testColor.inputX, testColor.inputY, testColor.inputZ,
        ir1, ir2, measuredR, measuredG, measuredB, *calibrationData);
    
    result.processingTime = micros() - startTime;
    result.measuredRGB = RGBColor(measuredR, measuredG, measuredB);
    result.measuredLAB = colorDifferenceEngine.rgbToLAB(result.measuredRGB, config.colorSpace);
    
    // Calculate color difference
    result.colorDiff = colorDifferenceEngine.calculateDeltaE2000(testColor.expectedLAB, result.measuredLAB);
    
    // Determine if test passed
    float tolerance = config.useStrictTolerance ? testColor.tolerance : config.globalTolerance;
    result.passed = result.colorDiff.deltaE2000 <= tolerance;
    
    // Calculate accuracy percentage
    result.accuracy = calculateColorAccuracy(result.colorDiff.deltaE2000, tolerance);
    
    // Add notes about conversion method
    if (conversionMethod == 2) {
        result.notes = "4-point tetrahedral interpolation";
    } else if (conversionMethod == 1) {
        result.notes = "2-point linear interpolation";
    } else {
        result.notes = "Fallback conversion";
    }
    
    return result;
}

// Calculate overall validation statistics
void ValidationTestSuite::calculateValidationStatistics(ValidationResults& results, const TestResult* results_array, int count) {
    results.totalTests = count;
    results.passedTests = 0;
    results.failedTests = 0;
    results.totalProcessingTime = 0;
    results.averageDeltaE = 0.0f;
    results.maxDeltaE = 0.0f;
    results.minDeltaE = 999.0f;
    
    float totalAccuracy = 0.0f;
    float totalDeltaE = 0.0f;
    
    for (int i = 0; i < count; i++) {
        const TestResult& test = results_array[i];
        
        if (test.passed) {
            results.passedTests++;
        } else {
            results.failedTests++;
        }
        
        totalAccuracy += test.accuracy;
        totalDeltaE += test.colorDiff.deltaE2000;
        results.totalProcessingTime += test.processingTime;
        
        if (test.colorDiff.deltaE2000 > results.maxDeltaE) {
            results.maxDeltaE = test.colorDiff.deltaE2000;
        }
        if (test.colorDiff.deltaE2000 < results.minDeltaE) {
            results.minDeltaE = test.colorDiff.deltaE2000;
        }
    }
    
    if (count > 0) {
        results.overallAccuracy = totalAccuracy / count;
        results.averageDeltaE = totalDeltaE / count;
    }
    
    results.qualityLevel = assessCalibrationQuality(results);
    results.calibrationValid = (results.getPassRate() >= 70.0f && results.averageDeltaE <= 5.0f);
    results.timestamp = millis();
}

// Assess overall calibration quality
String ValidationTestSuite::assessCalibrationQuality(const ValidationResults& results) const {
    float passRate = results.getPassRate();
    float avgDeltaE = results.averageDeltaE;
    
    if (passRate >= 95.0f && avgDeltaE <= 1.5f) {
        return "Excellent";
    } else if (passRate >= 85.0f && avgDeltaE <= 2.5f) {
        return "Good";
    } else if (passRate >= 70.0f && avgDeltaE <= 4.0f) {
        return "Acceptable";
    } else if (passRate >= 50.0f && avgDeltaE <= 6.0f) {
        return "Poor";
    } else {
        return "Unacceptable";
    }
}

// Perform complete validation test
ValidationResults ValidationTestSuite::performValidation() {
    Serial.println("=== Starting Complete Validation Test ===");
    Serial.println("Test Suite: " + config.testSuite);
    Serial.println("Test Colors: " + String(testColorCount));
    
    if (!colorConverter || !calibrationData) {
        Serial.println("ERROR: Validation system not properly initialized");
        ValidationResults emptyResults;
        return emptyResults;
    }
    
    // Perform all tests
    for (int i = 0; i < testColorCount && i < MAX_RESULTS; i++) {
        testResults[i] = performSingleColorTest(testColors[i]);
        
        Serial.println("Test " + String(i + 1) + "/" + String(testColorCount) + ": " + 
                      testColors[i].name + " - " + 
                      String(testResults[i].passed ? "PASS" : "FAIL") + 
                      " (ΔE: " + String(testResults[i].colorDiff.deltaE2000, 2) + ")");
    }
    
    // Calculate statistics
    calculateValidationStatistics(lastValidationResults, testResults, testColorCount);
    
    Serial.println("=== Validation Complete ===");
    Serial.println("Pass Rate: " + String(lastValidationResults.getPassRate(), 1) + "%");
    Serial.println("Average ΔE: " + String(lastValidationResults.averageDeltaE, 2));
    Serial.println("Quality Level: " + lastValidationResults.qualityLevel);
    
    return lastValidationResults;
}
