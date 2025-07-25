/**
 * @file CalibrationLightingManager.h
 * @brief Professional lighting and environmental consistency management for color calibration
 * 
 * This class ensures consistent lighting conditions throughout the calibration process,
 * which is critical for achieving professional-grade color accuracy. It manages LED
 * brightness locking, ambient light monitoring, and environmental stability validation.
 * 
 * Key Features:
 * - LED brightness consistency enforcement
 * - Ambient light change detection
 * - Environmental stability monitoring
 * - Calibration sequence state management
 * - Professional-grade validation thresholds
 * 
 * @author Enhanced Color Calibration System
 * @version 2.0
 * @date 2025-01-24
 */

#ifndef CALIBRATION_LIGHTING_MANAGER_H
#define CALIBRATION_LIGHTING_MANAGER_H

#include "Arduino.h"
#include "ColorScience.h"

/**
 * @brief Environmental conditions snapshot
 */
struct EnvironmentalSnapshot {
    uint8_t ledBrightness;          // LED brightness level (0-255)
    uint16_t ambientIR1;            // Ambient IR1 level
    uint16_t ambientIR2;            // Ambient IR2 level
    float ambientIRRatio;           // IR1/IR2 ratio for consistency
    float temperature;              // Temperature (if available)
    uint32_t timestamp;             // When snapshot was taken
    bool isValid;                   // Whether snapshot is valid
    
    EnvironmentalSnapshot() : ledBrightness(0), ambientIR1(0), ambientIR2(0), 
                             ambientIRRatio(1.0f), temperature(25.0f), 
                             timestamp(0), isValid(false) {}
    
    /**
     * @brief Calculate difference from another snapshot
     * @param other Other snapshot to compare against
     * @return Normalized difference (0.0 = identical, 1.0 = completely different)
     */
    float calculateDifference(const EnvironmentalSnapshot& other) const;
    
    /**
     * @brief Get debug string representation
     */
    String toString() const;
};

/**
 * @brief Calibration sequence state tracking
 */
enum CalibrationSequenceState {
    SEQUENCE_IDLE,              // No calibration in progress
    SEQUENCE_BLACK_PHASE,       // Black reference calibration
    SEQUENCE_WHITE_PHASE,       // White reference calibration (brightness locked)
    SEQUENCE_BLUE_PHASE,        // Blue reference calibration
    SEQUENCE_YELLOW_PHASE,      // Yellow reference calibration
    SEQUENCE_VALIDATION_PHASE,  // Validation phase
    SEQUENCE_COMPLETE          // Calibration sequence complete
};

/**
 * @brief Environmental stability validation results
 */
struct StabilityValidation {
    bool brightnessStable;      // LED brightness unchanged
    bool ambientStable;         // Ambient lighting stable
    bool temperatureStable;     // Temperature stable (if available)
    bool overallStable;         // Overall environmental stability
    float stabilityScore;       // Stability score (0.0-1.0)
    String issues;              // Description of any issues
    uint32_t validationTime;    // When validation was performed
    
    StabilityValidation() : brightnessStable(false), ambientStable(false), 
                           temperatureStable(true), overallStable(false), 
                           stabilityScore(0.0f), validationTime(0) {}
};

/**
 * @brief Professional lighting and environmental management for calibration
 */
class CalibrationLightingManager {
private:
    // Current environmental state
    EnvironmentalSnapshot baselineSnapshot;
    EnvironmentalSnapshot currentSnapshot;
    CalibrationSequenceState sequenceState;
    
    // Brightness locking state
    bool brightnessLocked;
    uint8_t lockedBrightness;
    uint32_t lockTimestamp;
    uint32_t lockDuration;
    
    // Environmental monitoring
    bool environmentalMonitoringEnabled;
    uint32_t lastMonitoringUpdate;
    static const uint32_t MONITORING_INTERVAL_MS = 5000; // 5 seconds
    
    // Validation thresholds
    struct ValidationThresholds {
        float maxBrightnessChange = 0.0f;           // No brightness change allowed
        float maxAmbientIRChange = 0.15f;           // 15% ambient IR change threshold
        float maxTemperatureChange = 3.0f;          // 3°C temperature change threshold
        float minStabilityScore = 0.85f;            // Minimum stability score
        uint32_t maxCalibrationDuration = 600000;   // 10 minutes max calibration time
        uint32_t stabilizationTime = 2000;          // 2 seconds LED stabilization time
    } thresholds;
    
    // Statistics and monitoring
    uint32_t brightnessChangeCount;
    uint32_t ambientChangeCount;
    uint32_t stabilityViolationCount;
    uint32_t totalValidations;
    
    /**
     * @brief Update current environmental snapshot
     * @param ir1 Current IR1 reading
     * @param ir2 Current IR2 reading
     * @param temperature Current temperature (optional)
     */
    void updateCurrentSnapshot(uint16_t ir1, uint16_t ir2, float temperature = 25.0f);
    
    /**
     * @brief Validate environmental stability against baseline
     */
    StabilityValidation validateStability() const;
    
    /**
     * @brief Get sequence state name for debugging
     */
    String getSequenceStateName() const;
    
public:
    /**
     * @brief Constructor
     */
    CalibrationLightingManager();
    
    /**
     * @brief Initialize lighting manager
     * @param enableMonitoring Enable continuous environmental monitoring
     * @return true if initialization successful
     */
    bool initialize(bool enableMonitoring = true);
    
    /**
     * @brief Start calibration sequence and establish baseline
     * @param currentBrightness Current LED brightness
     * @param ir1 Current IR1 reading
     * @param ir2 Current IR2 reading
     * @param temperature Current temperature (optional)
     * @return true if sequence started successfully
     */
    bool startCalibrationSequence(uint8_t currentBrightness, uint16_t ir1, uint16_t ir2, 
                                 float temperature = 25.0f);
    
    /**
     * @brief Lock LED brightness for calibration consistency
     * @param brightness Brightness level to lock (0-255)
     * @return true if brightness locked successfully
     */
    bool lockBrightness(uint8_t brightness);
    
    /**
     * @brief Validate brightness consistency
     * @param currentBrightness Current LED brightness to validate
     * @return true if brightness is consistent with locked value
     */
    bool validateBrightnessConsistency(uint8_t currentBrightness);
    
    /**
     * @brief Validate environmental consistency
     * @param ir1 Current IR1 reading
     * @param ir2 Current IR2 reading
     * @param temperature Current temperature (optional)
     * @return Stability validation results
     */
    StabilityValidation validateEnvironmentalConsistency(uint16_t ir1, uint16_t ir2, 
                                                         float temperature = 25.0f);
    
    /**
     * @brief Advance to next calibration phase
     * @param nextState Next calibration phase
     * @return true if phase transition successful
     */
    bool advanceCalibrationPhase(CalibrationSequenceState nextState);
    
    /**
     * @brief Complete calibration sequence and unlock brightness
     */
    void completeCalibrationSequence();
    
    /**
     * @brief Abort calibration sequence due to environmental issues
     * @param reason Reason for aborting
     */
    void abortCalibrationSequence(const String& reason);
    
    /**
     * @brief Check if brightness is currently locked
     */
    bool isBrightnessLocked() const { return brightnessLocked; }
    
    /**
     * @brief Get locked brightness value
     */
    uint8_t getLockedBrightness() const { return lockedBrightness; }
    
    /**
     * @brief Get current calibration sequence state
     */
    CalibrationSequenceState getSequenceState() const { return sequenceState; }
    
    /**
     * @brief Get lock duration in milliseconds
     */
    uint32_t getLockDuration() const;
    
    /**
     * @brief Get environmental monitoring statistics
     */
    void getMonitoringStatistics(uint32_t& brightnessChanges, uint32_t& ambientChanges, 
                                uint32_t& stabilityViolations, uint32_t& totalValidations) const;
    
    /**
     * @brief Reset monitoring statistics
     */
    void resetStatistics();
    
    /**
     * @brief Get current environmental snapshot
     */
    EnvironmentalSnapshot getCurrentSnapshot() const { return currentSnapshot; }
    
    /**
     * @brief Get baseline environmental snapshot
     */
    EnvironmentalSnapshot getBaselineSnapshot() const { return baselineSnapshot; }
    
    /**
     * @brief Update validation thresholds
     * @param maxAmbientChange Maximum allowed ambient IR change (0.0-1.0)
     * @param maxTempChange Maximum allowed temperature change (°C)
     * @param minStability Minimum required stability score (0.0-1.0)
     */
    void updateValidationThresholds(float maxAmbientChange, float maxTempChange, float minStability);
    
    /**
     * @brief Get comprehensive debug information
     */
    String getDebugInfo() const;
    
    /**
     * @brief Perform environmental monitoring update (call periodically)
     * @param ir1 Current IR1 reading
     * @param ir2 Current IR2 reading
     * @param temperature Current temperature (optional)
     */
    void performMonitoringUpdate(uint16_t ir1, uint16_t ir2, float temperature = 25.0f);
    
    /**
     * @brief Check if environmental conditions are suitable for calibration
     * @param ir1 Current IR1 reading
     * @param ir2 Current IR2 reading
     * @param temperature Current temperature (optional)
     * @return true if conditions are suitable
     */
    bool areConditionsSuitableForCalibration(uint16_t ir1, uint16_t ir2, float temperature = 25.0f);
    
    /**
     * @brief Generate environmental report for calibration validation
     */
    String generateEnvironmentalReport() const;
};

#endif // CALIBRATION_LIGHTING_MANAGER_H
