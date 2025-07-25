/**
 * @file ValidationTestSuite.h
 * @brief Comprehensive validation test suite for color calibration systems
 * 
 * This class provides a complete validation framework for testing color
 * calibration accuracy using standardized test colors, CIEDE2000 color
 * difference calculations, and professional quality metrics. It includes
 * predefined test color sets based on industry standards.
 * 
 * Key Features:
 * - Standardized test color sets (Macbeth ColorChecker, IT8, custom)
 * - CIEDE2000-based accuracy testing
 * - Comprehensive quality metrics and reporting
 * - Batch validation with statistical analysis
 * - Performance benchmarking
 * - Calibration quality assessment
 * - Export capabilities for analysis
 * 
 * Test Color Sets:
 * - Basic RGB primaries and secondaries
 * - Macbeth ColorChecker 24-patch simulation
 * - Grayscale ramp for linearity testing
 * - Saturated colors for gamut testing
 * - Custom test colors for specific applications
 * 
 * @author Enhanced Color Calibration System
 * @version 2.0
 * @date 2025-01-24
 */

#ifndef VALIDATION_TEST_SUITE_H
#define VALIDATION_TEST_SUITE_H

#include "Arduino.h"
#include "CIEDE2000.h"
#include "ColorConversionEnhanced.h"
#include "ColorScience.h"

/**
 * @brief Test color definition
 */
struct TestColor {
    String name;                // Color name
    RGBColor expectedRGB;       // Expected RGB output
    LABColor expectedLAB;       // Expected LAB values
    uint16_t inputX, inputY, inputZ;  // Input XYZ sensor values
    float tolerance;            // Acceptable CIEDE2000 tolerance
    String category;            // Color category (primary, secondary, etc.)
    
    TestColor() : tolerance(3.0f), category("unknown") {}
    
    TestColor(const String& colorName, const RGBColor& rgb, uint16_t x, uint16_t y, uint16_t z,
              float tol = 3.0f, const String& cat = "custom")
        : name(colorName), expectedRGB(rgb), inputX(x), inputY(y), inputZ(z), 
          tolerance(tol), category(cat) {}
};

/**
 * @brief Individual test result
 */
struct TestResult {
    TestColor testColor;        // Test color definition
    RGBColor measuredRGB;       // Measured RGB output
    LABColor measuredLAB;       // Measured LAB values
    ColorDifferenceResult colorDiff;  // CIEDE2000 analysis
    bool passed;                // Whether test passed
    float accuracy;             // Accuracy percentage (0-100)
    uint32_t processingTime;    // Processing time in microseconds
    String notes;               // Additional notes
    
    TestResult() : passed(false), accuracy(0.0f), processingTime(0) {}
};

/**
 * @brief Validation test suite results
 */
struct ValidationResults {
    uint32_t totalTests;        // Total number of tests
    uint32_t passedTests;       // Number of passed tests
    uint32_t failedTests;       // Number of failed tests
    float overallAccuracy;      // Overall accuracy percentage
    float averageDeltaE;        // Average CIEDE2000 difference
    float maxDeltaE;            // Maximum CIEDE2000 difference
    float minDeltaE;            // Minimum CIEDE2000 difference
    uint32_t totalProcessingTime; // Total processing time
    String qualityLevel;        // Overall quality assessment
    bool calibrationValid;      // Whether calibration is valid
    uint32_t timestamp;         // When validation was performed
    
    ValidationResults() : totalTests(0), passedTests(0), failedTests(0),
                         overallAccuracy(0.0f), averageDeltaE(0.0f),
                         maxDeltaE(0.0f), minDeltaE(999.0f),
                         totalProcessingTime(0), calibrationValid(false),
                         timestamp(0) {}
    
    float getPassRate() const { return totalTests > 0 ? (float)passedTests / totalTests * 100.0f : 0.0f; }
    float getFailRate() const { return totalTests > 0 ? (float)failedTests / totalTests * 100.0f : 0.0f; }
};

/**
 * @brief Test suite configuration
 */
struct ValidationConfig {
    String testSuite;           // Test suite name
    float globalTolerance;      // Global CIEDE2000 tolerance
    bool useStrictTolerance;    // Use strict per-color tolerances
    bool includePerformance;    // Include performance testing
    bool generateReport;        // Generate detailed report
    String colorSpace;          // Color space for testing ("sRGB", "AdobeRGB")
    String application;         // Application type for tolerance assessment
    
    ValidationConfig() : testSuite("basic"), globalTolerance(3.0f),
                        useStrictTolerance(false), includePerformance(true),
                        generateReport(true), colorSpace("sRGB"),
                        application("general") {}
};

/**
 * @brief Comprehensive validation test suite
 */
class ValidationTestSuite {
private:
    CIEDE2000 colorDifferenceEngine;
    ColorConversionEnhanced* colorConverter;
    ColorScience::CalibrationData* calibrationData;
    
    // Test color sets
    static const int MAX_TEST_COLORS = 50;
    TestColor testColors[MAX_TEST_COLORS];
    int testColorCount;
    
    // Results storage
    static const int MAX_RESULTS = 50;
    TestResult testResults[MAX_RESULTS];
    ValidationResults lastValidationResults;
    
    // Configuration
    ValidationConfig config;
    
    /**
     * @brief Initialize basic RGB test colors
     */
    void initializeBasicTestColors();
    
    /**
     * @brief Initialize Macbeth ColorChecker simulation
     */
    void initializeMacbethColorChecker();
    
    /**
     * @brief Initialize grayscale test colors
     */
    void initializeGrayscaleColors();
    
    /**
     * @brief Initialize saturated test colors
     */
    void initializeSaturatedColors();
    
    /**
     * @brief Perform single color test
     */
    TestResult performSingleColorTest(const TestColor& testColor);
    
    /**
     * @brief Calculate overall validation statistics
     */
    void calculateValidationStatistics(ValidationResults& results, const TestResult* results_array, int count);
    
    /**
     * @brief Assess overall calibration quality
     */
    String assessCalibrationQuality(const ValidationResults& results) const;
    
public:
    /**
     * @brief Constructor
     */
    ValidationTestSuite();
    
    /**
     * @brief Initialize validation test suite
     * @param converter Color conversion system
     * @param calibData Calibration data
     * @return true if initialization successful
     */
    bool initialize(ColorConversionEnhanced* converter, ColorScience::CalibrationData* calibData);
    
    /**
     * @brief Load predefined test suite
     * @param suiteName Test suite name ("basic", "macbeth", "grayscale", "saturated", "comprehensive")
     * @return true if suite loaded successfully
     */
    bool loadTestSuite(const String& suiteName);
    
    /**
     * @brief Add custom test color
     * @param testColor Test color definition
     * @return true if color added successfully
     */
    bool addTestColor(const TestColor& testColor);
    
    /**
     * @brief Clear all test colors
     */
    void clearTestColors();
    
    /**
     * @brief Set validation configuration
     * @param validationConfig Configuration settings
     */
    void setValidationConfig(const ValidationConfig& validationConfig);
    
    /**
     * @brief Perform complete validation test
     * @return Validation results
     */
    ValidationResults performValidation();
    
    /**
     * @brief Perform quick validation test (subset of colors)
     * @param maxColors Maximum number of colors to test
     * @return Validation results
     */
    ValidationResults performQuickValidation(int maxColors = 10);
    
    /**
     * @brief Test specific color category
     * @param category Color category to test ("primary", "secondary", "grayscale", etc.)
     * @return Validation results for category
     */
    ValidationResults testColorCategory(const String& category);
    
    /**
     * @brief Validate calibration linearity using grayscale
     * @return Linearity validation results
     */
    ValidationResults validateLinearity();
    
    /**
     * @brief Validate color gamut coverage
     * @return Gamut validation results
     */
    ValidationResults validateGamut();
    
    /**
     * @brief Benchmark conversion performance
     * @param iterations Number of iterations for benchmarking
     * @return Performance benchmark results
     */
    ValidationResults benchmarkPerformance(int iterations = 1000);
    
    /**
     * @brief Get last validation results
     */
    ValidationResults getLastValidationResults() const { return lastValidationResults; }
    
    /**
     * @brief Get individual test results
     * @param results Array to store results (must be pre-allocated)
     * @param maxResults Maximum number of results to return
     * @return Number of results returned
     */
    int getIndividualTestResults(TestResult* results, int maxResults) const;
    
    /**
     * @brief Generate detailed validation report
     * @param includeIndividualResults Include individual test results
     * @return Detailed validation report
     */
    String generateValidationReport(bool includeIndividualResults = true) const;
    
    /**
     * @brief Export validation results as JSON
     * @param includeRawData Include raw measurement data
     * @return JSON formatted results
     */
    String exportValidationResults(bool includeRawData = false) const;
    
    /**
     * @brief Get validation recommendations
     * @return Recommendations for improving calibration
     */
    String getValidationRecommendations() const;
    
    /**
     * @brief Compare two validation results
     * @param results1 First validation results
     * @param results2 Second validation results
     * @return Comparison analysis
     */
    String compareValidationResults(const ValidationResults& results1, const ValidationResults& results2) const;
    
    /**
     * @brief Get test color count
     */
    int getTestColorCount() const { return testColorCount; }
    
    /**
     * @brief Get test color by index
     * @param index Test color index
     * @return Test color (or empty if invalid index)
     */
    TestColor getTestColor(int index) const;
    
    /**
     * @brief Find test color by name
     * @param name Color name
     * @return Test color index (-1 if not found)
     */
    int findTestColor(const String& name) const;
    
    /**
     * @brief Update CIEDE2000 quality thresholds
     * @param excellent Threshold for excellent quality
     * @param good Threshold for good quality
     * @param acceptable Threshold for acceptable quality
     * @param poor Threshold for poor quality
     */
    void updateQualityThresholds(float excellent, float good, float acceptable, float poor);
    
    /**
     * @brief Get current validation configuration
     */
    ValidationConfig getValidationConfig() const { return config; }
    
    /**
     * @brief Validate calibration data integrity
     * @return true if calibration data is valid
     */
    bool validateCalibrationData() const;
    
    /**
     * @brief Get validation statistics summary
     */
    String getValidationSummary() const;
    
    /**
     * @brief Reset validation statistics
     */
    void resetValidationStatistics();
};

/**
 * @brief Global validation test suite instance
 */
extern ValidationTestSuite validationSuite;

/**
 * @brief Utility functions for validation testing
 */

/**
 * @brief Create test color from RGB values
 * @param name Color name
 * @param r Red value (0-255)
 * @param g Green value (0-255)
 * @param b Blue value (0-255)
 * @param x Expected X sensor value
 * @param y Expected Y sensor value
 * @param z Expected Z sensor value
 * @param tolerance CIEDE2000 tolerance
 * @return Test color definition
 */
TestColor createTestColor(const String& name, uint8_t r, uint8_t g, uint8_t b,
                         uint16_t x, uint16_t y, uint16_t z, float tolerance = 3.0f);

/**
 * @brief Calculate color accuracy percentage from CIEDE2000 difference
 * @param deltaE CIEDE2000 color difference
 * @param maxAcceptable Maximum acceptable difference
 * @return Accuracy percentage (0-100)
 */
float calculateColorAccuracy(float deltaE, float maxAcceptable = 5.0f);

/**
 * @brief Generate quality assessment from validation results
 * @param results Validation results
 * @return Quality assessment string
 */
String generateQualityAssessment(const ValidationResults& results);

#endif // VALIDATION_TEST_SUITE_H
