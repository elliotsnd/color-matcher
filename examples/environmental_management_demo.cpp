/**
 * @file environmental_management_demo.cpp
 * @brief Comprehensive demonstration of environmental management for color calibration
 * 
 * This example demonstrates:
 * - CalibrationLightingManager for brightness consistency
 * - Environmental monitoring and stability validation
 * - Pre-calibration environmental checks
 * - Continuous monitoring during calibration
 * - Environmental alert system
 * - Integration with calibration workflow
 */

#include "EnvironmentalIntegration.h"
#include "CalibrationLightingManager.h"
#include "Arduino.h"

// Global environmental system
EnvironmentalIntegration envSystem;

// Simulated sensor readings for demonstration
struct SimulatedSensorData {
    uint16_t ir1, ir2;
    float temperature;
    uint8_t ledBrightness;
    
    SimulatedSensorData(uint16_t i1 = 500, uint16_t i2 = 600, float temp = 25.0f, uint8_t led = 20) 
        : ir1(i1), ir2(i2), temperature(temp), ledBrightness(led) {}
};

// Simulate environmental changes for testing
SimulatedSensorData simulateEnvironmentalConditions(int scenario) {
    switch (scenario) {
        case 0: // Ideal conditions
            return SimulatedSensorData(500, 600, 25.0f, 20);
        case 1: // Slight ambient light increase
            return SimulatedSensorData(650, 750, 25.5f, 20);
        case 2: // Moderate ambient light change
            return SimulatedSensorData(800, 950, 26.0f, 20);
        case 3: // Significant ambient light change
            return SimulatedSensorData(1200, 1400, 27.0f, 20);
        case 4: // Temperature change
            return SimulatedSensorData(500, 600, 30.0f, 20);
        case 5: // Brightness change (should trigger error)
            return SimulatedSensorData(500, 600, 25.0f, 25);
        case 6: // High ambient light (unsuitable)
            return SimulatedSensorData(25000, 28000, 25.0f, 20);
        case 7: // Very low readings (sensor issue)
            return SimulatedSensorData(5, 8, 25.0f, 20);
        default:
            return SimulatedSensorData();
    }
}

void demonstrateLightingManager() {
    Serial.println("\n=== Lighting Manager Demonstration ===");
    
    CalibrationLightingManager lightingMgr;
    
    // Initialize lighting manager
    if (!lightingMgr.initialize(true)) {
        Serial.println("ERROR: Failed to initialize lighting manager");
        return;
    }
    
    // Simulate calibration sequence
    SimulatedSensorData initialConditions = simulateEnvironmentalConditions(0);
    
    Serial.println("Starting calibration sequence...");
    if (!lightingMgr.startCalibrationSequence(initialConditions.ledBrightness, 
                                             initialConditions.ir1, initialConditions.ir2, 
                                             initialConditions.temperature)) {
        Serial.println("ERROR: Failed to start calibration sequence");
        return;
    }
    
    // Simulate black reference phase
    Serial.println("\nPhase 1: Black Reference");
    lightingMgr.advanceCalibrationPhase(SEQUENCE_BLACK_PHASE);
    
    // Simulate white reference phase with brightness locking
    Serial.println("\nPhase 2: White Reference (Locking brightness)");
    if (lightingMgr.lockBrightness(20)) {
        Serial.println("Brightness locked successfully");
    }
    lightingMgr.advanceCalibrationPhase(SEQUENCE_WHITE_PHASE);
    
    // Test brightness consistency validation
    Serial.println("\nTesting brightness consistency...");
    
    // Test with correct brightness
    if (lightingMgr.validateBrightnessConsistency(20)) {
        Serial.println("✓ Brightness consistency validated");
    } else {
        Serial.println("✗ Brightness consistency failed");
    }
    
    // Test with incorrect brightness
    if (lightingMgr.validateBrightnessConsistency(25)) {
        Serial.println("✓ Brightness consistency validated (unexpected)");
    } else {
        Serial.println("✗ Brightness consistency failed (expected)");
    }
    
    // Simulate blue and yellow phases
    Serial.println("\nPhase 3: Blue Reference");
    lightingMgr.advanceCalibrationPhase(SEQUENCE_BLUE_PHASE);
    
    Serial.println("\nPhase 4: Yellow Reference");
    lightingMgr.advanceCalibrationPhase(SEQUENCE_YELLOW_PHASE);
    
    // Test environmental stability with changing conditions
    Serial.println("\nTesting environmental stability...");
    
    for (int scenario = 0; scenario < 4; scenario++) {
        SimulatedSensorData conditions = simulateEnvironmentalConditions(scenario);
        StabilityValidation validation = lightingMgr.validateEnvironmentalConsistency(
            conditions.ir1, conditions.ir2, conditions.temperature);
        
        Serial.println("Scenario " + String(scenario) + ":");
        Serial.println("  Stable: " + String(validation.overallStable ? "Yes" : "No"));
        Serial.println("  Score: " + String(validation.stabilityScore, 3));
        if (!validation.overallStable) {
            Serial.println("  Issues: " + validation.issues);
        }
    }
    
    // Complete calibration
    Serial.println("\nCompleting calibration sequence...");
    lightingMgr.completeCalibrationSequence();
    
    // Display statistics
    uint32_t brightnessChanges, ambientChanges, stabilityViolations, totalValidations;
    lightingMgr.getMonitoringStatistics(brightnessChanges, ambientChanges, 
                                       stabilityViolations, totalValidations);
    
    Serial.println("\nLighting Manager Statistics:");
    Serial.println("  Brightness Changes: " + String(brightnessChanges));
    Serial.println("  Ambient Changes: " + String(ambientChanges));
    Serial.println("  Stability Violations: " + String(stabilityViolations));
    Serial.println("  Total Validations: " + String(totalValidations));
    Serial.println("  Lock Duration: " + String(lightingMgr.getLockDuration()) + "ms");
}

void demonstrateEnvironmentalIntegration() {
    Serial.println("\n=== Environmental Integration Demonstration ===");
    
    // Initialize environmental system
    if (!envSystem.initialize(true, false)) {
        Serial.println("ERROR: Failed to initialize environmental system");
        return;
    }
    
    // Perform pre-calibration check
    Serial.println("\nPerforming pre-calibration environmental check...");
    SimulatedSensorData conditions = simulateEnvironmentalConditions(0);
    
    PreCalibrationCheck preCheck = envSystem.performPreCalibrationCheck(
        conditions.ir1, conditions.ir2, conditions.temperature);
    
    Serial.println("Pre-calibration check results:");
    Serial.println("  Suitable: " + String(preCheck.suitable ? "Yes" : "No"));
    Serial.println("  Current Stability: " + String(preCheck.currentStability, 3));
    Serial.println("  Predicted Stability: " + String(preCheck.predictedStability, 3));
    if (!preCheck.suitable) {
        Serial.println("  Issues: " + preCheck.issues);
        Serial.println("  Recommendations: " + preCheck.recommendations);
    }
    
    if (preCheck.suitable) {
        // Start environmental calibration
        Serial.println("\nStarting environmental calibration...");
        if (envSystem.startEnvironmentalCalibration(conditions.ledBrightness, 
                                                   conditions.ir1, conditions.ir2, 
                                                   conditions.temperature)) {
            Serial.println("Environmental calibration started successfully");
            
            // Simulate calibration steps with environmental validation
            Serial.println("\nValidating black reference conditions...");
            if (envSystem.validateBlackReferenceConditions(conditions.ir1, conditions.ir2, conditions.temperature)) {
                Serial.println("✓ Black reference conditions suitable");
            }
            
            Serial.println("\nValidating white reference conditions...");
            if (envSystem.validateWhiteReferenceConditions(20, conditions.ir1, conditions.ir2, conditions.temperature)) {
                Serial.println("✓ White reference conditions suitable");
            }
            
            Serial.println("\nValidating blue reference conditions...");
            if (envSystem.validateBlueReferenceConditions(conditions.ir1, conditions.ir2, conditions.temperature)) {
                Serial.println("✓ Blue reference conditions suitable");
            }
            
            Serial.println("\nValidating yellow reference conditions...");
            if (envSystem.validateYellowReferenceConditions(conditions.ir1, conditions.ir2, conditions.temperature)) {
                Serial.println("✓ Yellow reference conditions suitable");
            }
            
            // Simulate continuous monitoring during calibration
            Serial.println("\nSimulating continuous monitoring with changing conditions...");
            for (int i = 0; i < 5; i++) {
                SimulatedSensorData monitoringConditions = simulateEnvironmentalConditions(i);
                bool suitable = envSystem.performContinuousMonitoring(
                    monitoringConditions.ir1, monitoringConditions.ir2, 
                    monitoringConditions.temperature, monitoringConditions.ledBrightness);
                
                Serial.println("Monitoring step " + String(i + 1) + ": " + 
                              String(suitable ? "Suitable" : "Issues detected"));
                
                if (!suitable) {
                    Serial.println("  Environmental alert level: " + 
                                  String(envSystem.getCurrentEnvironmentalStatus()));
                }
            }
            
            // Complete calibration
            Serial.println("\nCompleting environmental calibration...");
            EnvironmentalCalibrationResult result = envSystem.completeEnvironmentalCalibration();
            
            Serial.println("Calibration result:");
            Serial.println("  Success: " + String(result.success ? "Yes" : "No"));
            Serial.println("  Stability Score: " + String(result.stabilityScore, 3));
            Serial.println("  Duration: " + String(result.calibrationDuration) + "ms");
            Serial.println("  Environmental Issues: " + String(result.environmentalIssues));
            if (!result.success) {
                Serial.println("  Error: " + result.errorMessage);
            }
        }
    }
    
    // Display integration statistics
    uint32_t successful, failed, envAborts;
    float successRate;
    envSystem.getIntegrationStatistics(successful, failed, envAborts, successRate);
    
    Serial.println("\nEnvironmental Integration Statistics:");
    Serial.println("  Successful Calibrations: " + String(successful));
    Serial.println("  Failed Calibrations: " + String(failed));
    Serial.println("  Environmental Aborts: " + String(envAborts));
    Serial.println("  Success Rate: " + String(successRate, 1) + "%");
}

void demonstrateEnvironmentalAlerts() {
    Serial.println("\n=== Environmental Alert System Demonstration ===");
    
    // Test various environmental scenarios
    String scenarios[] = {
        "Ideal conditions",
        "Slight ambient change",
        "Moderate ambient change", 
        "Significant ambient change",
        "Temperature change",
        "Brightness change",
        "High ambient light",
        "Very low readings"
    };
    
    for (int i = 0; i < 8; i++) {
        Serial.println("\nTesting scenario: " + scenarios[i]);
        SimulatedSensorData conditions = simulateEnvironmentalConditions(i);
        
        // Perform environmental check
        PreCalibrationCheck check = checkEnvironmentalSuitability(
            conditions.ir1, conditions.ir2, conditions.temperature);
        
        Serial.println("  Suitable: " + String(check.suitable ? "Yes" : "No"));
        Serial.println("  Stability: " + String(check.currentStability, 3));
        if (!check.suitable) {
            Serial.println("  Issues: " + check.issues);
            Serial.println("  Recommendations: " + check.recommendations);
        }
        
        // Perform environmental update to trigger monitoring
        performEnvironmentalUpdate(conditions.ir1, conditions.ir2, 
                                  conditions.temperature, conditions.ledBrightness);
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("Environmental Management System Demo");
    Serial.println("====================================");
    Serial.println("This demo showcases the comprehensive environmental");
    Serial.println("management system for professional color calibration.\n");
    
    // Run demonstrations
    demonstrateLightingManager();
    demonstrateEnvironmentalIntegration();
    demonstrateEnvironmentalAlerts();
    
    Serial.println("\n=== Demo Complete ===");
    Serial.println("The environmental management system provides:");
    Serial.println("✓ LED brightness consistency enforcement");
    Serial.println("✓ Ambient light stability monitoring");
    Serial.println("✓ Temperature change detection");
    Serial.println("✓ Pre-calibration environmental validation");
    Serial.println("✓ Continuous monitoring during calibration");
    Serial.println("✓ Automatic alert system for environmental issues");
    Serial.println("✓ Professional-grade environmental reporting");
    Serial.println("\nThe system is ready for integration with your calibration workflow!");
}

void loop() {
    // Demo complete - just wait
    delay(10000);
}
