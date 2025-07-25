/**
 * @file CalibrationLightingManager.cpp
 * @brief Implementation of professional lighting and environmental management
 */

#include "CalibrationLightingManager.h"

// EnvironmentalSnapshot methods
float EnvironmentalSnapshot::calculateDifference(const EnvironmentalSnapshot& other) const {
    if (!isValid || !other.isValid) {
        return 1.0f; // Maximum difference for invalid snapshots
    }
    
    // Calculate normalized differences
    float brightnessDiff = abs(ledBrightness - other.ledBrightness) / 255.0f;
    
    float ambientIR1Diff = 0.0f;
    float ambientIR2Diff = 0.0f;
    if (other.ambientIR1 > 0) {
        ambientIR1Diff = abs((int)ambientIR1 - (int)other.ambientIR1) / (float)other.ambientIR1;
    }
    if (other.ambientIR2 > 0) {
        ambientIR2Diff = abs((int)ambientIR2 - (int)other.ambientIR2) / (float)other.ambientIR2;
    }
    
    float temperatureDiff = abs(temperature - other.temperature) / 50.0f; // Normalize to 50°C range
    
    // Weighted combination (brightness is most critical)
    float totalDiff = (brightnessDiff * 0.5f) + 
                     (ambientIR1Diff * 0.2f) + 
                     (ambientIR2Diff * 0.2f) + 
                     (temperatureDiff * 0.1f);
    
    return min(totalDiff, 1.0f);
}

String EnvironmentalSnapshot::toString() const {
    String result = "Environmental Snapshot:\n";
    result += "  LED Brightness: " + String(ledBrightness) + "\n";
    result += "  Ambient IR1: " + String(ambientIR1) + "\n";
    result += "  Ambient IR2: " + String(ambientIR2) + "\n";
    result += "  IR Ratio: " + String(ambientIRRatio, 3) + "\n";
    result += "  Temperature: " + String(temperature, 1) + "°C\n";
    result += "  Timestamp: " + String(timestamp) + "\n";
    result += "  Valid: " + String(isValid ? "Yes" : "No");
    return result;
}

// CalibrationLightingManager implementation
CalibrationLightingManager::CalibrationLightingManager() {
    brightnessLocked = false;
    lockedBrightness = 0;
    lockTimestamp = 0;
    lockDuration = 0;
    sequenceState = SEQUENCE_IDLE;
    environmentalMonitoringEnabled = false;
    lastMonitoringUpdate = 0;
    
    // Initialize statistics
    brightnessChangeCount = 0;
    ambientChangeCount = 0;
    stabilityViolationCount = 0;
    totalValidations = 0;
}

bool CalibrationLightingManager::initialize(bool enableMonitoring) {
    Serial.println("=== Initializing Calibration Lighting Manager ===");
    
    environmentalMonitoringEnabled = enableMonitoring;
    sequenceState = SEQUENCE_IDLE;
    
    // Reset all state
    brightnessLocked = false;
    lockedBrightness = 0;
    lockTimestamp = 0;
    
    // Clear snapshots
    baselineSnapshot = EnvironmentalSnapshot();
    currentSnapshot = EnvironmentalSnapshot();
    
    Serial.println("Lighting manager initialized");
    Serial.println("Environmental monitoring: " + String(enableMonitoring ? "Enabled" : "Disabled"));
    Serial.println("Validation thresholds:");
    Serial.println("  Max ambient IR change: " + String(thresholds.maxAmbientIRChange * 100) + "%");
    Serial.println("  Max temperature change: " + String(thresholds.maxTemperatureChange) + "°C");
    Serial.println("  Min stability score: " + String(thresholds.minStabilityScore));
    
    return true;
}

bool CalibrationLightingManager::startCalibrationSequence(uint8_t currentBrightness, uint16_t ir1, 
                                                         uint16_t ir2, float temperature) {
    Serial.println("=== Starting Calibration Sequence ===");
    
    if (sequenceState != SEQUENCE_IDLE) {
        Serial.println("ERROR: Calibration sequence already in progress");
        return false;
    }
    
    // Establish baseline environmental conditions
    baselineSnapshot.ledBrightness = currentBrightness;
    baselineSnapshot.ambientIR1 = ir1;
    baselineSnapshot.ambientIR2 = ir2;
    baselineSnapshot.ambientIRRatio = (ir2 > 0) ? (float)ir1 / ir2 : 1.0f;
    baselineSnapshot.temperature = temperature;
    baselineSnapshot.timestamp = millis();
    baselineSnapshot.isValid = true;
    
    // Initialize current snapshot
    currentSnapshot = baselineSnapshot;
    
    // Start with black phase (no brightness locking yet)
    sequenceState = SEQUENCE_BLACK_PHASE;
    
    Serial.println("Calibration sequence started");
    Serial.println("Baseline conditions established:");
    Serial.println("  LED Brightness: " + String(currentBrightness));
    Serial.println("  Ambient IR1: " + String(ir1));
    Serial.println("  Ambient IR2: " + String(ir2));
    Serial.println("  Temperature: " + String(temperature, 1) + "°C");
    
    return true;
}

bool CalibrationLightingManager::lockBrightness(uint8_t brightness) {
    if (brightnessLocked && brightness != lockedBrightness) {
        Serial.println("ERROR: Brightness already locked at " + String(lockedBrightness) + 
                      ", cannot change to " + String(brightness));
        return false;
    }
    
    if (!brightnessLocked) {
        lockedBrightness = brightness;
        brightnessLocked = true;
        lockTimestamp = millis();
        
        Serial.println("LED brightness locked at " + String(brightness) + " for calibration sequence");
        
        // Advance to white phase when brightness is first locked
        if (sequenceState == SEQUENCE_BLACK_PHASE) {
            sequenceState = SEQUENCE_WHITE_PHASE;
        }
    }
    
    return true;
}

bool CalibrationLightingManager::validateBrightnessConsistency(uint8_t currentBrightness) {
    totalValidations++;
    
    if (!brightnessLocked) {
        return true; // No validation needed if not locked
    }
    
    if (currentBrightness != lockedBrightness) {
        brightnessChangeCount++;
        Serial.println("ERROR: Brightness inconsistency detected!");
        Serial.println("  Expected: " + String(lockedBrightness));
        Serial.println("  Current: " + String(currentBrightness));
        Serial.println("  Lock duration: " + String(getLockDuration()) + "ms");
        return false;
    }
    
    return true;
}

void CalibrationLightingManager::updateCurrentSnapshot(uint16_t ir1, uint16_t ir2, float temperature) {
    currentSnapshot.ambientIR1 = ir1;
    currentSnapshot.ambientIR2 = ir2;
    currentSnapshot.ambientIRRatio = (ir2 > 0) ? (float)ir1 / ir2 : 1.0f;
    currentSnapshot.temperature = temperature;
    currentSnapshot.timestamp = millis();
    currentSnapshot.isValid = true;
    
    // LED brightness should be updated externally when changed
}

StabilityValidation CalibrationLightingManager::validateStability() const {
    StabilityValidation validation;
    validation.validationTime = millis();
    
    if (!baselineSnapshot.isValid || !currentSnapshot.isValid) {
        validation.issues = "Invalid environmental snapshots";
        return validation;
    }
    
    // Validate brightness stability
    validation.brightnessStable = (currentSnapshot.ledBrightness == baselineSnapshot.ledBrightness);
    
    // Validate ambient lighting stability
    float ambientIR1Change = 0.0f;
    float ambientIR2Change = 0.0f;
    
    if (baselineSnapshot.ambientIR1 > 0) {
        ambientIR1Change = abs((int)currentSnapshot.ambientIR1 - (int)baselineSnapshot.ambientIR1) / 
                          (float)baselineSnapshot.ambientIR1;
    }
    if (baselineSnapshot.ambientIR2 > 0) {
        ambientIR2Change = abs((int)currentSnapshot.ambientIR2 - (int)baselineSnapshot.ambientIR2) / 
                          (float)baselineSnapshot.ambientIR2;
    }
    
    float maxAmbientChange = max(ambientIR1Change, ambientIR2Change);
    validation.ambientStable = (maxAmbientChange <= thresholds.maxAmbientIRChange);
    
    // Validate temperature stability
    float temperatureChange = abs(currentSnapshot.temperature - baselineSnapshot.temperature);
    validation.temperatureStable = (temperatureChange <= thresholds.maxTemperatureChange);
    
    // Calculate overall stability score
    float stabilityFactors[] = {
        validation.brightnessStable ? 1.0f : 0.0f,
        validation.ambientStable ? 1.0f : (1.0f - maxAmbientChange),
        validation.temperatureStable ? 1.0f : (1.0f - temperatureChange / thresholds.maxTemperatureChange)
    };
    
    validation.stabilityScore = (stabilityFactors[0] * 0.5f + 
                               stabilityFactors[1] * 0.3f + 
                               stabilityFactors[2] * 0.2f);
    
    validation.overallStable = (validation.stabilityScore >= thresholds.minStabilityScore);
    
    // Generate issues description
    if (!validation.overallStable) {
        validation.issues = "";
        if (!validation.brightnessStable) {
            validation.issues += "LED brightness changed; ";
        }
        if (!validation.ambientStable) {
            validation.issues += "Ambient lighting changed by " + String(maxAmbientChange * 100, 1) + "%; ";
        }
        if (!validation.temperatureStable) {
            validation.issues += "Temperature changed by " + String(temperatureChange, 1) + "°C; ";
        }
    }
    
    return validation;
}

StabilityValidation CalibrationLightingManager::validateEnvironmentalConsistency(uint16_t ir1, uint16_t ir2, 
                                                                                float temperature) {
    // Update current snapshot
    updateCurrentSnapshot(ir1, ir2, temperature);
    
    // Perform stability validation
    StabilityValidation validation = validateStability();
    
    if (!validation.overallStable) {
        stabilityViolationCount++;
        Serial.println("Environmental stability violation detected:");
        Serial.println("  Stability score: " + String(validation.stabilityScore, 3));
        Serial.println("  Issues: " + validation.issues);
    }
    
    return validation;
}

bool CalibrationLightingManager::advanceCalibrationPhase(CalibrationSequenceState nextState) {
    String currentStateName = getSequenceStateName();
    sequenceState = nextState;
    String nextStateName = getSequenceStateName();
    
    Serial.println("Calibration phase advanced: " + currentStateName + " -> " + nextStateName);
    
    return true;
}

void CalibrationLightingManager::completeCalibrationSequence() {
    Serial.println("=== Completing Calibration Sequence ===");
    
    sequenceState = SEQUENCE_COMPLETE;
    brightnessLocked = false;
    lockDuration = getLockDuration();
    
    Serial.println("Calibration sequence completed successfully");
    Serial.println("Total lock duration: " + String(lockDuration) + "ms");
    Serial.println("Brightness changes detected: " + String(brightnessChangeCount));
    Serial.println("Ambient changes detected: " + String(ambientChangeCount));
    Serial.println("Stability violations: " + String(stabilityViolationCount));
}

void CalibrationLightingManager::abortCalibrationSequence(const String& reason) {
    Serial.println("=== Aborting Calibration Sequence ===");
    Serial.println("Reason: " + reason);
    
    sequenceState = SEQUENCE_IDLE;
    brightnessLocked = false;
    lockDuration = getLockDuration();
    
    Serial.println("Calibration sequence aborted");
}

uint32_t CalibrationLightingManager::getLockDuration() const {
    if (brightnessLocked && lockTimestamp > 0) {
        return millis() - lockTimestamp;
    } else if (lockDuration > 0) {
        return lockDuration; // Return stored duration from completed sequence
    }
    return 0;
}

String CalibrationLightingManager::getSequenceStateName() const {
    switch (sequenceState) {
        case SEQUENCE_IDLE: return "Idle";
        case SEQUENCE_BLACK_PHASE: return "Black Phase";
        case SEQUENCE_WHITE_PHASE: return "White Phase";
        case SEQUENCE_BLUE_PHASE: return "Blue Phase";
        case SEQUENCE_YELLOW_PHASE: return "Yellow Phase";
        case SEQUENCE_VALIDATION_PHASE: return "Validation Phase";
        case SEQUENCE_COMPLETE: return "Complete";
        default: return "Unknown";
    }
}

void CalibrationLightingManager::getMonitoringStatistics(uint32_t& brightnessChanges, uint32_t& ambientChanges, 
                                                         uint32_t& stabilityViolations, uint32_t& totalValidations) const {
    brightnessChanges = brightnessChangeCount;
    ambientChanges = ambientChangeCount;
    stabilityViolations = stabilityViolationCount;
    totalValidations = this->totalValidations;
}

void CalibrationLightingManager::resetStatistics() {
    brightnessChangeCount = 0;
    ambientChangeCount = 0;
    stabilityViolationCount = 0;
    totalValidations = 0;
    Serial.println("Environmental monitoring statistics reset");
}

void CalibrationLightingManager::updateValidationThresholds(float maxAmbientChange, float maxTempChange, 
                                                           float minStability) {
    thresholds.maxAmbientIRChange = constrain(maxAmbientChange, 0.0f, 1.0f);
    thresholds.maxTemperatureChange = constrain(maxTempChange, 0.0f, 50.0f);
    thresholds.minStabilityScore = constrain(minStability, 0.0f, 1.0f);
    
    Serial.println("Validation thresholds updated:");
    Serial.println("  Max ambient change: " + String(thresholds.maxAmbientIRChange * 100) + "%");
    Serial.println("  Max temperature change: " + String(thresholds.maxTemperatureChange) + "°C");
    Serial.println("  Min stability score: " + String(thresholds.minStabilityScore));
}

String CalibrationLightingManager::getDebugInfo() const {
    String info = "=== Calibration Lighting Manager Debug Info ===\n";
    info += "Sequence State: " + getSequenceStateName() + "\n";
    info += "Brightness Locked: " + String(brightnessLocked ? "Yes" : "No") + "\n";
    if (brightnessLocked) {
        info += "Locked Brightness: " + String(lockedBrightness) + "\n";
        info += "Lock Duration: " + String(getLockDuration()) + "ms\n";
    }
    info += "Environmental Monitoring: " + String(environmentalMonitoringEnabled ? "Enabled" : "Disabled") + "\n";

    info += "\nStatistics:\n";
    info += "  Brightness Changes: " + String(brightnessChangeCount) + "\n";
    info += "  Ambient Changes: " + String(ambientChangeCount) + "\n";
    info += "  Stability Violations: " + String(stabilityViolationCount) + "\n";
    info += "  Total Validations: " + String(totalValidations) + "\n";

    if (baselineSnapshot.isValid) {
        info += "\nBaseline Snapshot:\n";
        String baselineStr = baselineSnapshot.toString();
        baselineStr.replace("\n", "\n  ");
        info += "  " + baselineStr + "\n";
    }

    if (currentSnapshot.isValid) {
        info += "\nCurrent Snapshot:\n";
        String currentStr = currentSnapshot.toString();
        currentStr.replace("\n", "\n  ");
        info += "  " + currentStr + "\n";

        if (baselineSnapshot.isValid) {
            float difference = currentSnapshot.calculateDifference(baselineSnapshot);
            info += "  Difference from baseline: " + String(difference * 100, 1) + "%\n";
        }
    }

    info += "\nValidation Thresholds:\n";
    info += "  Max Ambient IR Change: " + String(thresholds.maxAmbientIRChange * 100) + "%\n";
    info += "  Max Temperature Change: " + String(thresholds.maxTemperatureChange) + "°C\n";
    info += "  Min Stability Score: " + String(thresholds.minStabilityScore) + "\n";

    return info;
}

void CalibrationLightingManager::performMonitoringUpdate(uint16_t ir1, uint16_t ir2, float temperature) {
    if (!environmentalMonitoringEnabled) {
        return;
    }

    uint32_t currentTime = millis();
    if (currentTime - lastMonitoringUpdate < MONITORING_INTERVAL_MS) {
        return; // Too soon for next update
    }

    lastMonitoringUpdate = currentTime;

    // Update current snapshot
    updateCurrentSnapshot(ir1, ir2, temperature);

    // Perform stability check if calibration is in progress
    if (sequenceState != SEQUENCE_IDLE && sequenceState != SEQUENCE_COMPLETE) {
        StabilityValidation validation = validateStability();

        if (!validation.overallStable) {
            Serial.println("Environmental monitoring alert: " + validation.issues);
        }
    }
}

bool CalibrationLightingManager::areConditionsSuitableForCalibration(uint16_t ir1, uint16_t ir2,
                                                                     float temperature) {
    // Update current conditions
    updateCurrentSnapshot(ir1, ir2, temperature);

    // Check basic suitability criteria
    bool suitable = true;
    String issues = "";

    // Check IR levels are reasonable (not too high ambient light)
    if (ir1 > 30000 || ir2 > 30000) {
        suitable = false;
        issues += "High ambient light detected; ";
    }

    // Check IR levels are not too low (sensor working)
    if (ir1 < 10 && ir2 < 10) {
        suitable = false;
        issues += "Very low IR readings, check sensor; ";
    }

    // Check temperature is in reasonable range
    if (temperature < 0 || temperature > 60) {
        suitable = false;
        issues += "Temperature out of range (" + String(temperature, 1) + "°C); ";
    }

    if (!suitable) {
        Serial.println("Conditions not suitable for calibration: " + issues);
    }

    return suitable;
}

String CalibrationLightingManager::generateEnvironmentalReport() const {
    String report = "=== Environmental Calibration Report ===\n";
    report += "Generated: " + String(millis()) + "ms\n";
    report += "Sequence State: " + getSequenceStateName() + "\n\n";

    // Calibration sequence summary
    if (sequenceState != SEQUENCE_IDLE) {
        report += "Calibration Sequence Summary:\n";
        report += "  Duration: " + String(getLockDuration()) + "ms\n";
        report += "  Brightness Locked: " + String(brightnessLocked ? "Yes" : "No") + "\n";
        if (brightnessLocked) {
            report += "  Locked Brightness: " + String(lockedBrightness) + "\n";
        }
        report += "\n";
    }

    // Environmental stability
    if (baselineSnapshot.isValid && currentSnapshot.isValid) {
        StabilityValidation validation = validateStability();
        report += "Environmental Stability:\n";
        report += "  Overall Stable: " + String(validation.overallStable ? "Yes" : "No") + "\n";
        report += "  Stability Score: " + String(validation.stabilityScore, 3) + "\n";
        report += "  Brightness Stable: " + String(validation.brightnessStable ? "Yes" : "No") + "\n";
        report += "  Ambient Stable: " + String(validation.ambientStable ? "Yes" : "No") + "\n";
        report += "  Temperature Stable: " + String(validation.temperatureStable ? "Yes" : "No") + "\n";
        if (!validation.overallStable) {
            report += "  Issues: " + validation.issues + "\n";
        }
        report += "\n";
    }

    // Statistics
    report += "Monitoring Statistics:\n";
    report += "  Brightness Changes: " + String(brightnessChangeCount) + "\n";
    report += "  Ambient Changes: " + String(ambientChangeCount) + "\n";
    report += "  Stability Violations: " + String(stabilityViolationCount) + "\n";
    report += "  Total Validations: " + String(totalValidations) + "\n";

    if (totalValidations > 0) {
        float violationRate = (float)stabilityViolationCount / totalValidations * 100.0f;
        report += "  Violation Rate: " + String(violationRate, 1) + "%\n";
    }

    return report;
}
