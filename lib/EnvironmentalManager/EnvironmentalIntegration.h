/**
 * @file EnvironmentalIntegration.h
 * @brief Integration layer for environmental management with main calibration system
 * 
 * This module provides the integration layer between the environmental management
 * system and the main color calibration system. It handles the coordination of
 * lighting management, environmental monitoring, and calibration processes.
 * 
 * @author Enhanced Color Calibration System
 * @version 2.0
 * @date 2025-01-24
 */

#ifndef ENVIRONMENTAL_INTEGRATION_H
#define ENVIRONMENTAL_INTEGRATION_H

#include "Arduino.h"
#include "CalibrationLightingManager.h"
#include "EnvironmentalMonitor.h"
#include "ColorScience.h"

/**
 * @brief Environmental calibration result
 */
struct EnvironmentalCalibrationResult {
    bool success;                       // Whether calibration was successful
    String errorMessage;                // Error message if failed
    EnvironmentalAlertLevel alertLevel; // Highest alert level during calibration
    float stabilityScore;               // Overall stability during calibration
    uint32_t calibrationDuration;       // Total calibration time
    uint32_t environmentalIssues;       // Number of environmental issues detected
    
    EnvironmentalCalibrationResult() : success(false), alertLevel(ALERT_NONE), 
                                      stabilityScore(0.0f), calibrationDuration(0), 
                                      environmentalIssues(0) {}
};

/**
 * @brief Environmental pre-calibration check results
 */
struct PreCalibrationCheck {
    bool suitable;                      // Whether conditions are suitable
    String issues;                      // Description of any issues
    String recommendations;             // Recommendations for improvement
    float currentStability;             // Current stability score
    float predictedStability;           // Predicted stability for calibration
    uint32_t recommendedDelay;          // Recommended delay before calibration (ms)
    
    PreCalibrationCheck() : suitable(false), currentStability(0.0f), 
                           predictedStability(0.0f), recommendedDelay(0) {}
};

/**
 * @brief Comprehensive environmental management integration
 */
class EnvironmentalIntegration {
private:
    CalibrationLightingManager lightingManager;
    EnvironmentalMonitor environmentalMonitor;
    
    // Integration state
    bool systemInitialized;
    bool calibrationInProgress;
    uint32_t calibrationStartTime;
    
    // Configuration
    bool strictEnvironmentalValidation;
    bool autoAbortOnEnvironmentalIssues;
    uint32_t maxCalibrationDuration;
    
    // Statistics
    uint32_t successfulCalibrations;
    uint32_t failedCalibrations;
    uint32_t environmentalAborts;
    
    /**
     * @brief Validate environmental conditions for calibration step
     */
    bool validateEnvironmentalConditionsForStep(CalibrationSequenceState step, 
                                               uint16_t ir1, uint16_t ir2, float temperature);
    
    /**
     * @brief Handle environmental alerts during calibration
     */
    bool handleEnvironmentalAlerts();
    
public:
    /**
     * @brief Constructor
     */
    EnvironmentalIntegration();
    
    /**
     * @brief Initialize environmental integration system
     * @param strictValidation Enable strict environmental validation
     * @param autoAbort Automatically abort calibration on environmental issues
     * @return true if initialization successful
     */
    bool initialize(bool strictValidation = true, bool autoAbort = false);
    
    /**
     * @brief Perform pre-calibration environmental check
     * @param ir1 Current IR1 reading
     * @param ir2 Current IR2 reading
     * @param temperature Current temperature
     * @return Pre-calibration check results
     */
    PreCalibrationCheck performPreCalibrationCheck(uint16_t ir1, uint16_t ir2, float temperature = 25.0f);
    
    /**
     * @brief Start environmentally-managed calibration sequence
     * @param currentBrightness Current LED brightness
     * @param ir1 Current IR1 reading
     * @param ir2 Current IR2 reading
     * @param temperature Current temperature
     * @return true if calibration sequence started successfully
     */
    bool startEnvironmentalCalibration(uint8_t currentBrightness, uint16_t ir1, uint16_t ir2, 
                                      float temperature = 25.0f);
    
    /**
     * @brief Validate environmental conditions for black reference calibration
     * @param ir1 Current IR1 reading
     * @param ir2 Current IR2 reading
     * @param temperature Current temperature
     * @return true if conditions are suitable
     */
    bool validateBlackReferenceConditions(uint16_t ir1, uint16_t ir2, float temperature = 25.0f);
    
    /**
     * @brief Validate environmental conditions for white reference calibration
     * @param brightness LED brightness to use
     * @param ir1 Current IR1 reading
     * @param ir2 Current IR2 reading
     * @param temperature Current temperature
     * @return true if conditions are suitable
     */
    bool validateWhiteReferenceConditions(uint8_t brightness, uint16_t ir1, uint16_t ir2, 
                                         float temperature = 25.0f);
    
    /**
     * @brief Validate environmental conditions for blue reference calibration
     * @param ir1 Current IR1 reading
     * @param ir2 Current IR2 reading
     * @param temperature Current temperature
     * @return true if conditions are suitable
     */
    bool validateBlueReferenceConditions(uint16_t ir1, uint16_t ir2, float temperature = 25.0f);
    
    /**
     * @brief Validate environmental conditions for yellow reference calibration
     * @param ir1 Current IR1 reading
     * @param ir2 Current IR2 reading
     * @param temperature Current temperature
     * @return true if conditions are suitable
     */
    bool validateYellowReferenceConditions(uint16_t ir1, uint16_t ir2, float temperature = 25.0f);
    
    /**
     * @brief Complete environmental calibration sequence
     * @return Calibration result with environmental analysis
     */
    EnvironmentalCalibrationResult completeEnvironmentalCalibration();
    
    /**
     * @brief Abort environmental calibration sequence
     * @param reason Reason for aborting
     * @return Calibration result with abort information
     */
    EnvironmentalCalibrationResult abortEnvironmentalCalibration(const String& reason);
    
    /**
     * @brief Perform continuous environmental monitoring during calibration
     * @param ir1 Current IR1 reading
     * @param ir2 Current IR2 reading
     * @param temperature Current temperature
     * @param ledBrightness Current LED brightness
     * @return true if environmental conditions remain suitable
     */
    bool performContinuousMonitoring(uint16_t ir1, uint16_t ir2, float temperature, uint8_t ledBrightness);
    
    /**
     * @brief Get current environmental status
     */
    EnvironmentalAlertLevel getCurrentEnvironmentalStatus() const;
    
    /**
     * @brief Get environmental stability score
     */
    float getEnvironmentalStabilityScore() const;
    
    /**
     * @brief Get lighting manager reference
     */
    CalibrationLightingManager& getLightingManager() { return lightingManager; }
    
    /**
     * @brief Get environmental monitor reference
     */
    EnvironmentalMonitor& getEnvironmentalMonitor() { return environmentalMonitor; }
    
    /**
     * @brief Check if calibration is currently in progress
     */
    bool isCalibrationInProgress() const { return calibrationInProgress; }
    
    /**
     * @brief Get calibration duration (if in progress)
     */
    uint32_t getCalibrationDuration() const;
    
    /**
     * @brief Get environmental integration statistics
     */
    void getIntegrationStatistics(uint32_t& successful, uint32_t& failed, 
                                 uint32_t& environmentalAborts, float& successRate) const;
    
    /**
     * @brief Reset integration statistics
     */
    void resetStatistics();
    
    /**
     * @brief Update environmental validation settings
     * @param strictValidation Enable strict validation
     * @param autoAbort Enable automatic abort on issues
     * @param maxDuration Maximum calibration duration (ms)
     */
    void updateValidationSettings(bool strictValidation, bool autoAbort, uint32_t maxDuration);
    
    /**
     * @brief Generate comprehensive environmental report
     */
    String generateEnvironmentalReport() const;
    
    /**
     * @brief Get debug information for environmental integration
     */
    String getDebugInfo() const;
    
    /**
     * @brief Export environmental data for analysis
     * @param includeAlerts Include alert history in export
     * @param includeDataPoints Include measurement data points
     * @return JSON formatted environmental data
     */
    String exportEnvironmentalData(bool includeAlerts = true, bool includeDataPoints = true) const;
};

// Global instance for easy integration
extern EnvironmentalIntegration environmentalSystem;

/**
 * @brief Enhanced calibration functions with environmental management
 */

/**
 * @brief Start environmentally-managed black reference calibration
 * @param ir1 Current IR1 reading
 * @param ir2 Current IR2 reading
 * @param temperature Current temperature
 * @return true if calibration can proceed
 */
bool startEnvironmentalBlackCalibration(uint16_t ir1, uint16_t ir2, float temperature = 25.0f);

/**
 * @brief Start environmentally-managed white reference calibration
 * @param brightness LED brightness to use
 * @param ir1 Current IR1 reading
 * @param ir2 Current IR2 reading
 * @param temperature Current temperature
 * @return true if calibration can proceed
 */
bool startEnvironmentalWhiteCalibration(uint8_t brightness, uint16_t ir1, uint16_t ir2, float temperature = 25.0f);

/**
 * @brief Start environmentally-managed blue reference calibration
 * @param ir1 Current IR1 reading
 * @param ir2 Current IR2 reading
 * @param temperature Current temperature
 * @return true if calibration can proceed
 */
bool startEnvironmentalBlueCalibration(uint16_t ir1, uint16_t ir2, float temperature = 25.0f);

/**
 * @brief Start environmentally-managed yellow reference calibration
 * @param ir1 Current IR1 reading
 * @param ir2 Current IR2 reading
 * @param temperature Current temperature
 * @return true if calibration can proceed
 */
bool startEnvironmentalYellowCalibration(uint16_t ir1, uint16_t ir2, float temperature = 25.0f);

/**
 * @brief Perform environmental monitoring update (call regularly from main loop)
 * @param ir1 Current IR1 reading
 * @param ir2 Current IR2 reading
 * @param temperature Current temperature
 * @param ledBrightness Current LED brightness
 */
void performEnvironmentalUpdate(uint16_t ir1, uint16_t ir2, float temperature, uint8_t ledBrightness);

/**
 * @brief Check if environmental conditions are suitable for any calibration
 * @param ir1 Current IR1 reading
 * @param ir2 Current IR2 reading
 * @param temperature Current temperature
 * @return Pre-calibration check results
 */
PreCalibrationCheck checkEnvironmentalSuitability(uint16_t ir1, uint16_t ir2, float temperature = 25.0f);

#endif // ENVIRONMENTAL_INTEGRATION_H
