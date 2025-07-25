/**
 * @file FieldTestingFramework.h
 * @brief Comprehensive field testing framework for real-world calibration validation
 * 
 * This class provides a complete framework for conducting field tests of color
 * calibration systems in real-world environments. It enables systematic testing
 * across different conditions, environments, and use cases to validate and
 * optimize calibration performance in actual deployment scenarios.
 * 
 * Key Features:
 * - Systematic field test planning and execution
 * - Multi-environment testing protocols
 * - Real-world performance validation
 * - Statistical analysis of field test results
 * - Automated test scheduling and execution
 * - Environmental condition correlation analysis
 * - Long-term stability testing
 * - Comparative analysis between different configurations
 * 
 * Test Types:
 * - Environmental stress testing (temperature, lighting variations)
 * - Long-term stability testing (drift analysis)
 * - Multi-user validation testing
 * - Application-specific testing
 * - Comparative configuration testing
 * - Robustness testing under adverse conditions
 * 
 * @author Enhanced Color Calibration System
 * @version 2.0
 * @date 2025-01-24
 */

#ifndef FIELD_TESTING_FRAMEWORK_H
#define FIELD_TESTING_FRAMEWORK_H

#include "Arduino.h"
#include "ThresholdOptimizer.h"
#include "ValidationTestSuite.h"
#include "EnvironmentalIntegration.h"

/**
 * @brief Field test configuration
 */
struct FieldTestConfig {
    String testName;            // Test name/identifier
    String testType;            // Type of test (stability, stress, comparative, etc.)
    uint32_t duration;          // Test duration in milliseconds
    uint32_t interval;          // Measurement interval in milliseconds
    uint32_t maxSamples;        // Maximum number of samples to collect
    String environment;         // Environment description
    String application;         // Application mode for testing
    bool enableOptimization;    // Whether to enable optimization during test
    bool enableValidation;      // Whether to perform validation during test
    String notes;               // Test notes and objectives
    
    FieldTestConfig() : duration(3600000), interval(60000), maxSamples(100),
                       environment("laboratory"), application("general"),
                       enableOptimization(false), enableValidation(true) {}
};

/**
 * @brief Field test measurement
 */
struct FieldTestMeasurement {
    uint32_t timestamp;         // Measurement timestamp
    uint32_t testTime;          // Time since test start
    float validationScore;      // Validation performance score
    float environmentalScore;   // Environmental stability score
    float conversionAccuracy;   // Color conversion accuracy
    uint16_t ambientIR1;        // Ambient IR1 level
    uint16_t ambientIR2;        // Ambient IR2 level
    float temperature;          // Temperature reading
    uint8_t ledBrightness;      // LED brightness level
    bool calibrationValid;      // Whether calibration was valid
    String notes;               // Measurement notes
    
    FieldTestMeasurement() : timestamp(0), testTime(0), validationScore(0),
                            environmentalScore(0), conversionAccuracy(0),
                            ambientIR1(0), ambientIR2(0), temperature(25.0f),
                            ledBrightness(0), calibrationValid(false) {}
};

/**
 * @brief Field test results summary
 */
struct FieldTestResults {
    String testName;            // Test identifier
    uint32_t startTime;         // Test start timestamp
    uint32_t endTime;           // Test end timestamp
    uint32_t actualDuration;    // Actual test duration
    uint32_t measurementCount;  // Number of measurements taken
    
    // Performance statistics
    float avgValidationScore;   // Average validation score
    float minValidationScore;   // Minimum validation score
    float maxValidationScore;   // Maximum validation score
    float validationStdDev;     // Validation score standard deviation
    
    float avgEnvironmentalScore; // Average environmental score
    float minEnvironmentalScore; // Minimum environmental score
    float maxEnvironmentalScore; // Maximum environmental score
    float environmentalStdDev;   // Environmental score standard deviation
    
    float avgConversionAccuracy; // Average conversion accuracy
    float minConversionAccuracy; // Minimum conversion accuracy
    float maxConversionAccuracy; // Maximum conversion accuracy
    float conversionStdDev;      // Conversion accuracy standard deviation
    
    // Environmental statistics
    float avgTemperature;       // Average temperature
    float minTemperature;       // Minimum temperature
    float maxTemperature;       // Maximum temperature
    float temperatureRange;     // Temperature range
    
    uint16_t avgAmbientIR1;     // Average ambient IR1
    uint16_t minAmbientIR1;     // Minimum ambient IR1
    uint16_t maxAmbientIR1;     // Maximum ambient IR1
    
    // Quality metrics
    float stabilityScore;       // Overall stability score
    float reliabilityScore;     // Overall reliability score
    float robustnessScore;      // Overall robustness score
    uint32_t validMeasurements; // Number of valid measurements
    float validMeasurementRate; // Percentage of valid measurements
    
    bool testCompleted;         // Whether test completed successfully
    String testSummary;         // Test summary and conclusions
    
    FieldTestResults() : startTime(0), endTime(0), actualDuration(0), measurementCount(0),
                        avgValidationScore(0), minValidationScore(999), maxValidationScore(0),
                        validationStdDev(0), avgEnvironmentalScore(0), minEnvironmentalScore(999),
                        maxEnvironmentalScore(0), environmentalStdDev(0), avgConversionAccuracy(0),
                        minConversionAccuracy(999), maxConversionAccuracy(0), conversionStdDev(0),
                        avgTemperature(0), minTemperature(999), maxTemperature(0), temperatureRange(0),
                        avgAmbientIR1(0), minAmbientIR1(65535), maxAmbientIR1(0),
                        stabilityScore(0), reliabilityScore(0), robustnessScore(0),
                        validMeasurements(0), validMeasurementRate(0), testCompleted(false) {}
};

/**
 * @brief Comprehensive field testing framework
 */
class FieldTestingFramework {
private:
    // Dependencies
    ThresholdOptimizer* thresholdOptimizer;
    ValidationTestSuite* validationSuite;
    EnvironmentalIntegration* environmentalSystem;
    
    // Current test state
    bool testInProgress;
    FieldTestConfig currentTestConfig;
    uint32_t testStartTime;
    uint32_t lastMeasurementTime;
    
    // Test data storage
    static const int MAX_MEASUREMENTS = 200;
    FieldTestMeasurement measurements[MAX_MEASUREMENTS];
    int measurementIndex;
    int measurementCount;
    
    // Test results storage
    static const int MAX_TEST_RESULTS = 10;
    FieldTestResults testResults[MAX_TEST_RESULTS];
    int testResultsIndex;
    int testResultsCount;
    
    // Statistics
    uint32_t totalTestsRun;
    uint32_t successfulTests;
    float averageStabilityScore;
    float bestStabilityScore;
    
    /**
     * @brief Perform single measurement during field test
     */
    FieldTestMeasurement performMeasurement();
    
    /**
     * @brief Calculate test statistics from measurements
     */
    void calculateTestStatistics(FieldTestResults& results);
    
    /**
     * @brief Analyze measurement trends and patterns
     */
    void analyzeMeasurementTrends(FieldTestResults& results);
    
    /**
     * @brief Generate test conclusions and recommendations
     */
    String generateTestConclusions(const FieldTestResults& results);
    
    /**
     * @brief Add measurement to storage
     */
    void addMeasurement(const FieldTestMeasurement& measurement);
    
    /**
     * @brief Store test results
     */
    void storeTestResults(const FieldTestResults& results);
    
public:
    /**
     * @brief Constructor
     */
    FieldTestingFramework();
    
    /**
     * @brief Initialize field testing framework
     * @param optimizer Threshold optimizer
     * @param validationSys Validation test suite
     * @param envSystem Environmental integration system
     * @return true if initialization successful
     */
    bool initialize(ThresholdOptimizer* optimizer, ValidationTestSuite* validationSys,
                   EnvironmentalIntegration* envSystem);
    
    /**
     * @brief Start field test with specified configuration
     * @param config Test configuration
     * @return true if test started successfully
     */
    bool startFieldTest(const FieldTestConfig& config);
    
    /**
     * @brief Stop current field test
     * @return Test results
     */
    FieldTestResults stopFieldTest();
    
    /**
     * @brief Update field test (call periodically during test)
     * @return true if test is still running
     */
    bool updateFieldTest();
    
    /**
     * @brief Check if field test is currently in progress
     */
    bool isTestInProgress() const { return testInProgress; }
    
    /**
     * @brief Get current test progress
     * @return Progress percentage (0-100)
     */
    float getTestProgress() const;
    
    /**
     * @brief Get current test status
     */
    String getTestStatus() const;
    
    /**
     * @brief Perform stability test over specified duration
     * @param duration Test duration in milliseconds
     * @param interval Measurement interval in milliseconds
     * @return Stability test results
     */
    FieldTestResults performStabilityTest(uint32_t duration = 3600000, uint32_t interval = 60000);
    
    /**
     * @brief Perform environmental stress test
     * @param stressType Type of stress test ("temperature", "lighting", "vibration")
     * @param duration Test duration in milliseconds
     * @return Stress test results
     */
    FieldTestResults performStressTest(const String& stressType, uint32_t duration = 1800000);
    
    /**
     * @brief Perform comparative test between configurations
     * @param configA First configuration name
     * @param configB Second configuration name
     * @param testDuration Duration for each configuration test
     * @return Comparative test results
     */
    FieldTestResults performComparativeTest(const String& configA, const String& configB,
                                           uint32_t testDuration = 1800000);
    
    /**
     * @brief Perform long-term drift analysis
     * @param duration Test duration in milliseconds (default: 24 hours)
     * @param interval Measurement interval in milliseconds
     * @return Drift analysis results
     */
    FieldTestResults performDriftAnalysis(uint32_t duration = 86400000, uint32_t interval = 300000);
    
    /**
     * @brief Perform robustness test under adverse conditions
     * @param adverseConditions Description of adverse conditions
     * @param duration Test duration in milliseconds
     * @return Robustness test results
     */
    FieldTestResults performRobustnessTest(const String& adverseConditions, uint32_t duration = 3600000);
    
    /**
     * @brief Get field test history
     * @param results Array to store test results (must be pre-allocated)
     * @param maxResults Maximum number of results to return
     * @return Number of results returned
     */
    int getTestHistory(FieldTestResults* results, int maxResults);
    
    /**
     * @brief Get current test measurements
     * @param measurements Array to store measurements (must be pre-allocated)
     * @param maxMeasurements Maximum number of measurements to return
     * @return Number of measurements returned
     */
    int getCurrentTestMeasurements(FieldTestMeasurement* measurements, int maxMeasurements);
    
    /**
     * @brief Generate comprehensive field test report
     * @param testName Specific test name (empty for all tests)
     * @return Detailed field test report
     */
    String generateFieldTestReport(const String& testName = "");
    
    /**
     * @brief Export field test data for analysis
     * @param format Export format ("json", "csv")
     * @param includeRawData Include raw measurement data
     * @return Exported field test data
     */
    String exportFieldTestData(const String& format = "json", bool includeRawData = true);
    
    /**
     * @brief Get field testing recommendations
     * @return Recommendations based on field test results
     */
    String getFieldTestingRecommendations();
    
    /**
     * @brief Compare field test results
     * @param testNameA First test name
     * @param testNameB Second test name
     * @return Comparison analysis
     */
    String compareFieldTestResults(const String& testNameA, const String& testNameB);
    
    /**
     * @brief Get field testing statistics
     */
    void getFieldTestingStatistics(uint32_t& totalTests, uint32_t& successfulTests,
                                  float& avgStability, float& bestStability) const;
    
    /**
     * @brief Reset field testing data
     */
    void resetFieldTestingData();
    
    /**
     * @brief Create predefined test configurations
     * @param testType Type of test ("quick", "stability", "stress", "comprehensive")
     * @return Test configuration
     */
    FieldTestConfig createPredefinedTestConfig(const String& testType);
    
    /**
     * @brief Validate field test configuration
     * @param config Configuration to validate
     * @return true if configuration is valid
     */
    bool validateTestConfig(const FieldTestConfig& config);
    
    /**
     * @brief Get debug information
     */
    String getDebugInfo();
    
    /**
     * @brief Schedule automatic field test
     * @param config Test configuration
     * @param startDelay Delay before starting test (milliseconds)
     * @return true if test scheduled successfully
     */
    bool scheduleFieldTest(const FieldTestConfig& config, uint32_t startDelay = 0);
    
    /**
     * @brief Cancel scheduled field test
     */
    void cancelScheduledTest();
    
    /**
     * @brief Check if test is scheduled
     */
    bool isTestScheduled() const;
};

/**
 * @brief Global field testing framework instance
 */
extern FieldTestingFramework fieldTestingFramework;

/**
 * @brief Utility functions for field testing
 */

/**
 * @brief Calculate statistical standard deviation
 * @param values Array of values
 * @param count Number of values
 * @param mean Mean value
 * @return Standard deviation
 */
float calculateStandardDeviation(const float* values, int count, float mean);

/**
 * @brief Calculate correlation coefficient between two data sets
 * @param dataA First data set
 * @param dataB Second data set
 * @param count Number of data points
 * @return Correlation coefficient (-1 to +1)
 */
float calculateCorrelation(const float* dataA, const float* dataB, int count);

/**
 * @brief Generate field test summary statistics
 * @param measurements Array of measurements
 * @param count Number of measurements
 * @return Statistical summary
 */
String generateStatisticalSummary(const FieldTestMeasurement* measurements, int count);

#endif // FIELD_TESTING_FRAMEWORK_H
