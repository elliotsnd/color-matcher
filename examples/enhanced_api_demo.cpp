/**
 * @file enhanced_api_demo.cpp
 * @brief Comprehensive demonstration of the enhanced API layer
 * 
 * This example demonstrates:
 * - Standardized APIResponse system
 * - Professional error handling and validation
 * - Structured JSON responses with metadata
 * - Enhanced calibration endpoint simulation
 * - Environmental integration with API responses
 * - Quality metrics and progress tracking
 */

#include "APIResponse.h"
#include "Arduino.h"

// Simulate calibration data for demonstration
struct DemoCalibrationData {
    bool blackComplete = false;
    bool whiteComplete = false;
    bool blueComplete = false;
    bool yellowComplete = false;
    uint8_t ledBrightness = 20;
    bool brightnessLocked = false;
    
    bool is2PointCalibrated() const { return blackComplete && whiteComplete; }
    bool is4PointCalibrated() const { return blackComplete && whiteComplete && blueComplete && yellowComplete; }
    int completedSteps() const { return blackComplete + whiteComplete + blueComplete + yellowComplete; }
    float progressPercent() const { return (completedSteps() / 4.0f) * 100.0f; }
};

DemoCalibrationData demoCalibData;

// Demonstrate basic APIResponse usage
void demonstrateBasicAPIResponse() {
    Serial.println("\n=== Basic APIResponse Demonstration ===");
    
    // Create JSON document
    PsramJsonAllocator allocator;
    JsonDocument doc = allocator.createDocument();
    
    // Create API response
    APIResponse response(doc, "/api/demo-endpoint", "demo-request-123");
    
    // Set success status
    response.setStatus(API_SUCCESS, "Demo operation completed successfully");
    
    // Add some data
    response.addData("demoValue", 42);
    response.addData("demoString", "Hello API");
    response.addData("demoBoolean", true);
    
    // Create nested data object
    JsonObject nestedData = response.createDataObject("nestedObject");
    nestedData["nestedValue"] = 3.14159;
    nestedData["nestedArray"] = JsonArray();
    nestedData["nestedArray"].add("item1");
    nestedData["nestedArray"].add("item2");
    
    // Add metadata
    response.setMetadata("customField", "customValue");
    response.setDebugMode(true);
    
    // Generate response
    String jsonResponse = response.toJSON(true); // Pretty print
    
    Serial.println("Basic API Response:");
    Serial.println(jsonResponse);
    Serial.println("Response size: " + String(response.getResponseSize()) + " bytes");
    Serial.println("HTTP status: " + String(response.getHTTPStatusCode()));
}

// Demonstrate error handling
void demonstrateErrorHandling() {
    Serial.println("\n=== Error Handling Demonstration ===");
    
    PsramJsonAllocator allocator;
    JsonDocument doc = allocator.createDocument();
    APIResponse response(doc, "/api/error-demo", "error-request-456");
    
    // Add various types of errors
    response.addError("VALIDATION_ERROR", "Required parameter missing", SEVERITY_ERROR, "brightness");
    response.addError("SENSOR_ERROR", "Sensor communication failed", SEVERITY_CRITICAL);
    response.addError("CALIBRATION_ERROR", "Calibration data invalid", SEVERITY_ERROR, "whiteReference");
    
    // Add warnings
    response.addWarning("PERFORMANCE_WARNING", "Operation took longer than expected", 
                       "Consider optimizing sensor readings");
    response.addWarning("ENVIRONMENTAL_WARNING", "Ambient light changed during operation",
                       "Ensure stable lighting conditions");
    
    // Add some context data
    response.addData("attemptedOperation", "calibrate-white");
    response.addData("sensorReadings", JsonObject());
    response.getData()["sensorReadings"]["X"] = 0; // Simulating failed reading
    response.getData()["sensorReadings"]["Y"] = 0;
    response.getData()["sensorReadings"]["Z"] = 0;
    
    String jsonResponse = response.toJSON(true);
    
    Serial.println("Error Response:");
    Serial.println(jsonResponse);
    Serial.println("Has errors: " + String(response.hasErrors() ? "Yes" : "No"));
    Serial.println("Has warnings: " + String(response.hasWarnings() ? "Yes" : "No"));
    Serial.println("HTTP status: " + String(response.getHTTPStatusCode()));
}

// Demonstrate calibration progress tracking
void demonstrateCalibrationProgress() {
    Serial.println("\n=== Calibration Progress Demonstration ===");
    
    // Simulate calibration steps
    String steps[] = {"black", "white", "blue", "yellow"};
    
    for (int i = 0; i <= 4; i++) {
        PsramJsonAllocator allocator;
        JsonDocument doc = allocator.createDocument();
        APIResponse response(doc, "/api/calibration-status", "progress-" + String(i));
        
        response.setStatus(API_SUCCESS, "Calibration status retrieved");
        
        // Add calibration progress
        String currentStep = (i < 4) ? steps[i] : "complete";
        String nextStep = (i < 3) ? steps[i + 1] : "";
        response.addCalibrationProgress(i, 4, currentStep, nextStep);
        
        // Add calibration data
        JsonObject calibData = response.createDataObject("calibrationData");
        calibData["blackComplete"] = (i >= 1);
        calibData["whiteComplete"] = (i >= 2);
        calibData["blueComplete"] = (i >= 3);
        calibData["yellowComplete"] = (i >= 4);
        calibData["is2PointReady"] = (i >= 2);
        calibData["is4PointReady"] = (i >= 4);
        
        String jsonResponse = response.toJSON();
        
        Serial.println("Step " + String(i) + " Progress:");
        Serial.println(jsonResponse);
        Serial.println();
    }
}

// Demonstrate environmental status integration
void demonstrateEnvironmentalStatus() {
    Serial.println("\n=== Environmental Status Demonstration ===");
    
    // Simulate different environmental scenarios
    struct EnvironmentalScenario {
        String name;
        float stabilityScore;
        String alertLevel;
        bool brightnessLocked;
        String issues;
    };
    
    EnvironmentalScenario scenarios[] = {
        {"Ideal Conditions", 0.98f, "none", true, ""},
        {"Minor Issues", 0.85f, "info", true, "Slight temperature variation"},
        {"Moderate Issues", 0.65f, "warning", true, "Ambient light changed by 15%"},
        {"Critical Issues", 0.30f, "critical", false, "Significant environmental instability"}
    };
    
    for (const auto& scenario : scenarios) {
        PsramJsonAllocator allocator;
        JsonDocument doc = allocator.createDocument();
        APIResponse response(doc, "/api/environmental-status", "env-" + scenario.name);
        
        response.setStatus(API_SUCCESS, "Environmental status retrieved");
        
        // Add environmental status
        response.addEnvironmentalStatus(scenario.stabilityScore, scenario.alertLevel,
                                       scenario.brightnessLocked, scenario.issues);
        
        // Add additional environmental data
        JsonObject envData = response.createDataObject("environmentalData");
        envData["ambientIR1"] = 500 + random(-100, 100);
        envData["ambientIR2"] = 600 + random(-100, 100);
        envData["temperature"] = 25.0f + random(-30, 30) / 10.0f;
        envData["calibrationDuration"] = random(30000, 300000);
        
        if (scenario.alertLevel != "none") {
            response.addWarning("ENVIRONMENTAL_CHANGE", scenario.issues,
                               "Monitor environmental conditions during calibration");
        }
        
        String jsonResponse = response.toJSON();
        
        Serial.println("Scenario: " + scenario.name);
        Serial.println(jsonResponse);
        Serial.println();
    }
}

// Demonstrate validation results
void demonstrateValidationResults() {
    Serial.println("\n=== Validation Results Demonstration ===");
    
    // Simulate different validation outcomes
    struct ValidationScenario {
        String name;
        bool passed;
        float accuracy;
        String details;
    };
    
    ValidationScenario scenarios[] = {
        {"Excellent Calibration", true, 96.5f, "All test colors within tolerance"},
        {"Good Calibration", true, 87.2f, "Minor deviations in blue spectrum"},
        {"Marginal Calibration", false, 68.8f, "Significant errors in yellow and red"},
        {"Poor Calibration", false, 45.3f, "Multiple critical failures detected"}
    };
    
    for (const auto& scenario : scenarios) {
        PsramJsonAllocator allocator;
        JsonDocument doc = allocator.createDocument();
        APIResponse response(doc, "/api/validate-calibration", "validation-" + scenario.name);
        
        if (scenario.passed) {
            response.setStatus(API_SUCCESS, "Calibration validation passed");
        } else {
            response.setStatus(API_WARNING, "Calibration validation completed with issues");
            response.addWarning("VALIDATION_ISSUES", "Calibration accuracy below threshold",
                               "Consider recalibrating with better reference materials");
        }
        
        // Add validation results
        response.addValidationResults(scenario.passed, scenario.accuracy, scenario.details);
        
        // Add detailed test results
        JsonArray testResults = response.createDataArray("testResults");
        String testColors[] = {"Red", "Green", "Blue", "Yellow", "Cyan", "Magenta"};
        
        for (const auto& color : testColors) {
            JsonObject testResult = testResults.createNestedObject();
            testResult["colorName"] = color;
            testResult["expectedRGB"] = JsonArray();
            testResult["measuredRGB"] = JsonArray();
            testResult["colorError"] = random(10, 200) / 10.0f; // Random error for demo
            testResult["passed"] = (testResult["colorError"].as<float>() < 10.0f);
        }
        
        String jsonResponse = response.toJSON();
        
        Serial.println("Validation: " + scenario.name);
        Serial.println(jsonResponse);
        Serial.println();
    }
}

// Demonstrate utility functions
void demonstrateUtilityFunctions() {
    Serial.println("\n=== Utility Functions Demonstration ===");
    
    PsramJsonAllocator allocator;
    JsonDocument doc = allocator.createDocument();
    
    // Test error response creation
    String errorResponse = APIResponse::createErrorResponse(doc, "DEMO_ERROR", 
                                                           "This is a demo error message", 
                                                           "/api/demo");
    Serial.println("Quick Error Response:");
    Serial.println(errorResponse);
    Serial.println();
    
    // Test success response creation
    doc.clear();
    String successResponse = APIResponse::createSuccessResponse(doc, "Demo operation successful", 
                                                               "/api/demo");
    Serial.println("Quick Success Response:");
    Serial.println(successResponse);
    Serial.println();
    
    // Test PSRAM allocator
    Serial.println("PSRAM Allocator Test:");
    Serial.println("Recommended size for 'simple': " + String(PsramJsonAllocator::getRecommendedSize("simple")));
    Serial.println("Recommended size for 'complex': " + String(PsramJsonAllocator::getRecommendedSize("complex")));
    Serial.println("Recommended size for 'data-heavy': " + String(PsramJsonAllocator::getRecommendedSize("data-heavy")));
    Serial.println("Recommended size for 'export': " + String(PsramJsonAllocator::getRecommendedSize("export")));
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("Enhanced API Layer Demo");
    Serial.println("=======================");
    Serial.println("This demo showcases the professional-grade API response system");
    Serial.println("with standardized error handling, structured responses, and");
    Serial.println("comprehensive metadata integration.\n");
    
    // Run demonstrations
    demonstrateBasicAPIResponse();
    demonstrateErrorHandling();
    demonstrateCalibrationProgress();
    demonstrateEnvironmentalStatus();
    demonstrateValidationResults();
    demonstrateUtilityFunctions();
    
    Serial.println("\n=== Demo Complete ===");
    Serial.println("The enhanced API layer provides:");
    Serial.println("✓ Standardized response structure");
    Serial.println("✓ Professional error handling with severity levels");
    Serial.println("✓ Structured data organization with nested objects");
    Serial.println("✓ Comprehensive metadata including timing and versioning");
    Serial.println("✓ Calibration progress tracking");
    Serial.println("✓ Environmental status integration");
    Serial.println("✓ Validation result reporting");
    Serial.println("✓ PSRAM-aware memory management");
    Serial.println("\nThe API layer is ready for integration with your web interface!");
}

void loop() {
    // Demo complete - just wait
    delay(10000);
}
