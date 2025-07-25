/**
 * @file optimization_tools_demo.cpp
 * @brief Comprehensive demonstration of empirical optimization tools
 * 
 * This example demonstrates:
 * - ThresholdOptimizer for parameter optimization
 * - Field data collection and analysis
 * - Automated threshold tuning based on real-world data
 * - Performance optimization and validation
 * - Field testing framework capabilities
 * - Statistical analysis and reporting
 */

#include "ThresholdOptimizer.h"
#include "FieldTestingFramework.h"
#include "ValidationTestSuite.h"
#include "EnvironmentalIntegration.h"
#include "ColorConversionEnhanced.h"
#include "Arduino.h"

// Global instances for demonstration
ThresholdOptimizer optimizer;
FieldTestingFramework fieldTesting;
ValidationTestSuite validationSuite;
EnvironmentalIntegration envSystem;
ColorConversionEnhanced colorConverter;

// Simulated calibration data for demonstration
ColorScience::CalibrationData demoCalibData;

// Initialize demo systems
void initializeDemoSystems() {
    Serial.println("=== Initializing Demo Systems ===");
    
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
    
    // Initialize systems
    envSystem.initialize(true, false);
    colorConverter.initialize(demoCalibData);
    validationSuite.initialize(&colorConverter, &demoCalibData);
    optimizer.initialize(&validationSuite, &envSystem, &demoCalibData);
    fieldTesting.initialize(&optimizer, &validationSuite, &envSystem);
    
    Serial.println("Demo systems initialized successfully");
}

// Demonstrate threshold optimizer
void demonstrateThresholdOptimizer() {
    Serial.println("\n=== Threshold Optimizer Demonstration ===");
    
    // Configure optimization
    OptimizationConfig config;
    config.algorithm = "gradient";
    config.maxIterations = 10;
    config.convergenceThreshold = 0.02f;
    config.enableSafetyLimits = true;
    config.optimizationTarget = "overall_quality";
    
    optimizer.setOptimizationConfig(config);
    
    // Collect some simulated field data
    Serial.println("Collecting simulated field data...");
    for (int i = 0; i < 15; i++) {
        float validationScore = 0.75f + (random(-10, 10) / 100.0f);
        float environmentalScore = 0.80f + (random(-15, 15) / 100.0f);
        float conversionAccuracy = 0.85f + (random(-10, 10) / 100.0f);
        uint16_t ambientIR1 = 500 + random(-50, 50);
        uint16_t ambientIR2 = 600 + random(-60, 60);
        float temperature = 25.0f + random(-30, 30) / 10.0f;
        uint8_t ledBrightness = 20;
        
        optimizer.collectFieldData(validationScore, environmentalScore, conversionAccuracy,
                                  ambientIR1, ambientIR2, temperature, ledBrightness, "demo");
        
        delay(100); // Simulate time between measurements
    }
    
    // Get optimization recommendations
    String recommendations = optimizer.getOptimizationRecommendations();
    Serial.println("Optimization Recommendations:");
    Serial.println(recommendations);
    
    // Perform optimization
    Serial.println("\nPerforming threshold optimization...");
    OptimizationSession session = optimizer.performOptimization();
    
    Serial.println("Optimization Session Results:");
    Serial.println("  Session ID: " + String(session.sessionId));
    Serial.println("  Type: " + session.optimizationType);
    Serial.println("  Iterations: " + String(session.iterationsRun));
    Serial.println("  Initial Score: " + String(session.initialScore, 3));
    Serial.println("  Final Score: " + String(session.finalScore, 3));
    Serial.println("  Improvement: " + String(session.improvement * 100, 1) + "%");
    Serial.println("  Successful: " + String(session.successful ? "Yes" : "No"));
    Serial.println("  Notes: " + session.notes);
    
    // Show parameter status
    Serial.println("\nOptimization Parameter Status:");
    OptimizationParameter params[20];
    int paramCount = optimizer.getAllParameters(params, 20);
    
    for (int i = 0; i < paramCount; i++) {
        Serial.println("Parameter: " + params[i].name);
        Serial.println("  Current Value: " + String(params[i].currentValue, 3));
        Serial.println("  Best Value: " + String(params[i].bestValue, 3));
        Serial.println("  Best Score: " + String(params[i].bestScore, 3));
        Serial.println("  Optimized: " + String(params[i].isOptimized ? "Yes" : "No"));
        Serial.println("  Category: " + params[i].category);
        Serial.println();
    }
}

// Demonstrate A/B testing
void demonstrateABTesting() {
    Serial.println("\n=== A/B Testing Demonstration ===");
    
    // Test different validation tolerance values
    String parameterName = "validation_tolerance";
    float valueA = 2.5f;
    float valueB = 3.5f;
    int testSamples = 5;
    
    Serial.println("A/B Testing Parameter: " + parameterName);
    Serial.println("Value A: " + String(valueA, 1));
    Serial.println("Value B: " + String(valueB, 1));
    Serial.println("Test Samples: " + String(testSamples));
    
    float abResult = optimizer.performABTesting(parameterName, valueA, valueB, testSamples);
    
    Serial.println("A/B Test Results:");
    if (abResult > 0.02f) {
        Serial.println("  Result: Value B is significantly better");
        Serial.println("  Improvement: " + String(abResult * 100, 1) + "%");
        Serial.println("  Recommendation: Use Value B (" + String(valueB, 1) + ")");
    } else if (abResult < -0.02f) {
        Serial.println("  Result: Value A is significantly better");
        Serial.println("  Improvement: " + String(-abResult * 100, 1) + "%");
        Serial.println("  Recommendation: Use Value A (" + String(valueA, 1) + ")");
    } else {
        Serial.println("  Result: No significant difference");
        Serial.println("  Difference: " + String(abResult * 100, 1) + "%");
        Serial.println("  Recommendation: Either value is acceptable");
    }
}

// Demonstrate field testing framework
void demonstrateFieldTesting() {
    Serial.println("\n=== Field Testing Framework Demonstration ===");
    
    // Create test configuration
    FieldTestConfig testConfig;
    testConfig.testName = "Demo_Stability_Test";
    testConfig.testType = "stability";
    testConfig.duration = 30000;  // 30 seconds for demo
    testConfig.interval = 2000;   // 2 seconds between measurements
    testConfig.maxSamples = 20;
    testConfig.environment = "laboratory";
    testConfig.application = "demo";
    testConfig.enableValidation = true;
    testConfig.notes = "Demonstration of field testing capabilities";
    
    Serial.println("Starting field test: " + testConfig.testName);
    Serial.println("Duration: " + String(testConfig.duration / 1000) + " seconds");
    Serial.println("Interval: " + String(testConfig.interval / 1000) + " seconds");
    
    if (fieldTesting.startFieldTest(testConfig)) {
        Serial.println("Field test started successfully");
        
        // Monitor test progress
        while (fieldTesting.isTestInProgress()) {
            fieldTesting.updateFieldTest();
            
            float progress = fieldTesting.getTestProgress();
            String status = fieldTesting.getTestStatus();
            
            Serial.println("Test Progress: " + String(progress, 1) + "% - " + status);
            
            delay(3000); // Update every 3 seconds for demo
        }
        
        // Get test results
        FieldTestResults results = fieldTesting.stopFieldTest();
        
        Serial.println("\nField Test Results:");
        Serial.println("  Test Name: " + results.testName);
        Serial.println("  Duration: " + String(results.actualDuration / 1000) + " seconds");
        Serial.println("  Measurements: " + String(results.measurementCount));
        Serial.println("  Test Completed: " + String(results.testCompleted ? "Yes" : "No"));
        
        Serial.println("\nPerformance Statistics:");
        Serial.println("  Avg Validation Score: " + String(results.avgValidationScore, 3));
        Serial.println("  Min Validation Score: " + String(results.minValidationScore, 3));
        Serial.println("  Max Validation Score: " + String(results.maxValidationScore, 3));
        Serial.println("  Validation Std Dev: " + String(results.validationStdDev, 3));
        
        Serial.println("  Avg Environmental Score: " + String(results.avgEnvironmentalScore, 3));
        Serial.println("  Environmental Std Dev: " + String(results.environmentalStdDev, 3));
        
        Serial.println("\nQuality Metrics:");
        Serial.println("  Stability Score: " + String(results.stabilityScore, 3));
        Serial.println("  Reliability Score: " + String(results.reliabilityScore, 3));
        Serial.println("  Robustness Score: " + String(results.robustnessScore, 3));
        Serial.println("  Valid Measurement Rate: " + String(results.validMeasurementRate, 1) + "%");
        
        Serial.println("\nTest Summary:");
        Serial.println(results.testSummary);
        
    } else {
        Serial.println("ERROR: Failed to start field test");
    }
}

// Demonstrate predefined test configurations
void demonstratePredefinedTests() {
    Serial.println("\n=== Predefined Test Configurations ===");
    
    String testTypes[] = {"quick", "stability", "stress", "comprehensive"};
    
    for (const auto& testType : testTypes) {
        FieldTestConfig config = fieldTesting.createPredefinedTestConfig(testType);
        
        Serial.println("Test Type: " + testType);
        Serial.println("  Name: " + config.testName);
        Serial.println("  Duration: " + String(config.duration / 1000) + " seconds");
        Serial.println("  Interval: " + String(config.interval / 1000) + " seconds");
        Serial.println("  Max Samples: " + String(config.maxSamples));
        Serial.println("  Environment: " + config.environment);
        Serial.println("  Enable Optimization: " + String(config.enableOptimization ? "Yes" : "No"));
        Serial.println("  Enable Validation: " + String(config.enableValidation ? "Yes" : "No"));
        Serial.println("  Notes: " + config.notes);
        Serial.println();
    }
}

// Demonstrate optimization statistics and reporting
void demonstrateOptimizationReporting() {
    Serial.println("\n=== Optimization Reporting Demonstration ===");
    
    // Get optimization statistics
    uint32_t totalOpts, successfulOpts;
    float avgImprovement, bestScore;
    optimizer.getOptimizationStatistics(totalOpts, successfulOpts, avgImprovement, bestScore);
    
    Serial.println("Optimization Statistics:");
    Serial.println("  Total Optimizations: " + String(totalOpts));
    Serial.println("  Successful Optimizations: " + String(successfulOpts));
    Serial.println("  Success Rate: " + String(totalOpts > 0 ? (float)successfulOpts / totalOpts * 100 : 0, 1) + "%");
    Serial.println("  Average Improvement: " + String(avgImprovement * 100, 1) + "%");
    Serial.println("  Best Overall Score: " + String(bestScore, 3));
    
    // Generate optimization report
    String optimizationReport = optimizer.generateOptimizationReport();
    Serial.println("\nOptimization Report:");
    Serial.println(optimizationReport);
    
    // Get field testing statistics
    uint32_t totalTests, successfulTests;
    float avgStability, bestStability;
    fieldTesting.getFieldTestingStatistics(totalTests, successfulTests, avgStability, bestStability);
    
    Serial.println("\nField Testing Statistics:");
    Serial.println("  Total Tests: " + String(totalTests));
    Serial.println("  Successful Tests: " + String(successfulTests));
    Serial.println("  Success Rate: " + String(totalTests > 0 ? (float)successfulTests / totalTests * 100 : 0, 1) + "%");
    Serial.println("  Average Stability: " + String(avgStability, 3));
    Serial.println("  Best Stability: " + String(bestStability, 3));
    
    // Generate field testing recommendations
    String fieldRecommendations = fieldTesting.getFieldTestingRecommendations();
    Serial.println("\nField Testing Recommendations:");
    Serial.println(fieldRecommendations);
}

// Demonstrate data export capabilities
void demonstrateDataExport() {
    Serial.println("\n=== Data Export Demonstration ===");
    
    // Export optimization data
    String optimizationData = optimizer.exportOptimizationData(true, true);
    Serial.println("Optimization Data Export (JSON):");
    Serial.println(optimizationData.substring(0, 500) + "..."); // Show first 500 characters
    
    // Export field testing data
    String fieldTestData = fieldTesting.exportFieldTestData("json", true);
    Serial.println("\nField Test Data Export (JSON):");
    Serial.println(fieldTestData.substring(0, 500) + "..."); // Show first 500 characters
    
    Serial.println("\nData export capabilities demonstrated");
    Serial.println("Full data can be exported for external analysis");
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("Empirical Optimization Tools Demo");
    Serial.println("=================================");
    Serial.println("This demo showcases the comprehensive optimization tools");
    Serial.println("for field-based calibration tuning and performance optimization.\n");
    
    // Initialize demo systems
    initializeDemoSystems();
    
    // Run demonstrations
    demonstrateThresholdOptimizer();
    demonstrateABTesting();
    demonstrateFieldTesting();
    demonstratePredefinedTests();
    demonstrateOptimizationReporting();
    demonstrateDataExport();
    
    Serial.println("\n=== Demo Complete ===");
    Serial.println("The empirical optimization tools provide:");
    Serial.println("✓ Intelligent threshold optimization based on field data");
    Serial.println("✓ A/B testing framework for parameter validation");
    Serial.println("✓ Comprehensive field testing capabilities");
    Serial.println("✓ Statistical analysis and performance tracking");
    Serial.println("✓ Automated optimization with safety constraints");
    Serial.println("✓ Real-world performance validation");
    Serial.println("✓ Data export for external analysis");
    Serial.println("✓ Self-optimizing calibration system");
    Serial.println("\nThe optimization tools complete the transformation into a");
    Serial.println("professional-grade, self-optimizing color calibration system!");
}

void loop() {
    // Demo complete - just wait
    delay(10000);
}
