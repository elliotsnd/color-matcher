/**
 * @file ColorCalibrationManager.h
 * @brief Main calibration manager for 5-Point Color Correction Matrix system
 * @author ESP32 Color Calibration System
 * @version 1.0.0
 * @date 2024
 * 
 * This file provides the main interface for managing color calibration,
 * including adding/updating calibration points, persistent storage,
 * and automatic CCM recalculation.
 */

#ifndef COLOR_CALIBRATION_MANAGER_H
#define COLOR_CALIBRATION_MANAGER_H

#include "CalibrationStructures.h"
#include "MatrixSolver.h"
#include <Preferences.h>
#include <vector>

/**
 * @brief Main calibration manager class
 * 
 * Manages the complete color calibration lifecycle including:
 * - Adding/updating calibration points
 * - Persistent storage using ESP32 Preferences
 * - Automatic CCM recalculation
 * - Color conversion with calibration
 */
class ColorCalibrationManager {
public:
    /**
     * @brief Constructor
     */
    ColorCalibrationManager();
    
    /**
     * @brief Destructor
     */
    ~ColorCalibrationManager();
    
    /**
     * @brief Initialize the calibration manager
     * @return true if initialization successful, false otherwise
     */
    bool initialize();
    
    /**
     * @brief Calibrate dark offset (LED OFF) - Stage 1 of professional calibration
     * @param rawX Raw X sensor reading with LED OFF
     * @param rawY Raw Y sensor reading with LED OFF
     * @param rawZ Raw Z sensor reading with LED OFF
     * @return true if successful, false otherwise
     */
    bool calibrateDarkOffset(uint16_t rawX, uint16_t rawY, uint16_t rawZ);

    /**
     * @brief Dynamic dark offset recalibration for auto-exposure systems
     * @param currentGain Current sensor gain setting
     * @param currentIntegrationTime Current integration time in ms
     * @return true if recalibration was performed and successful
     */
    bool recalibrateDarkOffsetIfNeeded(float currentGain, uint16_t currentIntegrationTime);

    /**
     * @brief Invalidate dark offset when sensor settings change
     * Call this whenever gain, integration time, or other sensor settings change
     */
    void invalidateDarkOffset();

    /**
     * @brief Calibrate black reference (LED ON with black sample) - Stage 2 of professional calibration
     * @param rawX Raw X sensor reading with LED ON and black reference
     * @param rawY Raw Y sensor reading with LED ON and black reference
     * @param rawZ Raw Z sensor reading with LED ON and black reference
     * @return true if successful, false otherwise
     */
    bool calibrateBlackReference(uint16_t rawX, uint16_t rawY, uint16_t rawZ);

    /**
     * @brief Add or update a calibration point
     * @param colorName Name of the color ("black", "white", "grey", "blue", "yellow")
     * @param rawX Raw X sensor reading
     * @param rawY Raw Y sensor reading
     * @param rawZ Raw Z sensor reading
     * @param quality Quality score (0.0-1.0)
     * @return true if successful, false otherwise
     */
    bool addOrUpdateCalibrationPoint(const String& colorName, uint16_t rawX, uint16_t rawY, uint16_t rawZ, float quality = 1.0f);
    
    /**
     * @brief Apply calibration correction using tiered approach (never fails)
     * @param rawX Raw X sensor reading
     * @param rawY Raw Y sensor reading
     * @param rawZ Raw Z sensor reading
     * @param r Output red value (0-255)
     * @param g Output green value (0-255)
     * @param b Output blue value (0-255)
     * @return true if calibration applied, false if using fallback
     */
    bool applyCalibrationCorrection(uint16_t rawX, uint16_t rawY, uint16_t rawZ, uint8_t& r, uint8_t& g, uint8_t& b);

    /**
     * @brief Check if 2-point calibration is available (black + white)
     * @return true if both black and white points are calibrated
     */
    bool isTwoPointCalibrated() const;

    /**
     * @brief Check if matrix calibration is available and valid
     * @return true if matrix is calculated and valid
     */
    bool isMatrixCalibrated() const;

    /**
     * @brief Start auto-calibration cycle
     * @return true if started successfully
     */
    bool startAutoCalibration();

    /**
     * @brief Get auto-calibration status
     * @return Current auto-calibration status
     */
    AutoCalibrationStatus getAutoCalibrationStatus() const;

    /**
     * @brief Advance to next color in auto-calibration
     * @return true if advanced successfully
     */
    bool autoCalibrationNext();

    /**
     * @brief Retry current color in auto-calibration
     * @return true if retry successful
     */
    bool autoCalibrationRetry();

    /**
     * @brief Skip current color in auto-calibration
     * @return true if skipped successfully
     */
    bool autoCalibrationSkip();

    /**
     * @brief Complete auto-calibration process
     * @return true if completed successfully
     */
    bool autoCalibrationComplete();

    /**
     * @brief Get color information by enum
     * @param color Calibration color enum
     * @param name Output color name
     * @param r Output red value
     * @param g Output green value
     * @param b Output blue value
     * @return true if color found
     */
    bool getColorInfo(CalibrationColor color, String& name, uint8_t& r, uint8_t& g, uint8_t& b) const;
    
    /**
     * @brief Get current calibration status
     * @return Calibration status structure
     */
    CalibrationStatus getCalibrationStatus() const;
    
    /**
     * @brief Get the current Color Correction Matrix
     * @return Color correction matrix
     */
    ColorCorrectionMatrix getColorCorrectionMatrix() const;
    
    /**
     * @brief Reset all calibration data
     * @return true if successful, false otherwise
     */
    bool resetCalibration();
    
    /**
     * @brief Save calibration data to persistent storage
     * @return true if successful, false otherwise
     */
    bool saveCalibrationData();
    
    /**
     * @brief Load calibration data from persistent storage
     * @return true if successful, false otherwise
     */
    bool loadCalibrationData();
    
    /**
     * @brief Get calibration points for debugging
     * @return Vector of all calibration points
     */
    std::vector<CalibrationPoint> getCalibrationPoints() const;

    /**
     * @brief Get dark offset calibration point
     * @return Dark offset calibration point
     */
    CalibrationPoint getDarkOffsetPoint() const { return darkOffsetPoint; }

    /**
     * @brief Get black reference calibration point
     * @return Black reference calibration point
     */
    CalibrationPoint getBlackRefPoint() const { return blackRefPoint; }

    /**
     * @brief Check if dark offset is calibrated
     * @return true if dark offset is calibrated
     */
    bool isDarkOffsetCalibrated() const { return darkOffsetCalibrated; }

    /**
     * @brief Check if black reference is calibrated
     * @return true if black reference is calibrated
     */
    bool isBlackRefCalibrated() const { return blackRefCalibrated; }

    /**
     * @brief Get last error message
     * @return Error message string
     */
    String getLastError() const { return lastError; }

private:
    Preferences preferences;            ///< ESP32 Preferences for persistent storage

    // Enhanced calibration data with dark current and flare compensation
    CalibrationPoint darkOffsetPoint;  ///< Dark current reading (LED OFF)
    CalibrationPoint blackRefPoint;    ///< Flare black reading (LED ON with black reference)
    std::vector<CalibrationPoint> points; ///< Color calibration points

    ColorCorrectionMatrix ccm;          ///< Current color correction matrix
    MatrixSolver solver;                ///< Matrix solver instance
    String lastError;                   ///< Last error message
    bool isInitialized;                 ///< Initialization flag
    bool darkOffsetCalibrated;          ///< Dark offset calibration flag
    bool blackRefCalibrated;            ///< Black reference calibration flag

    // Dynamic calibration tracking for auto-exposure systems
    float lastCalibrationGain;          ///< Gain setting when dark offset was last calibrated
    uint16_t lastCalibrationIntegrationTime; ///< Integration time when dark offset was last calibrated
    bool sensorSettingsChanged;         ///< Flag indicating sensor settings have changed

    // Auto-calibration state
    AutoCalibrationStatus autoCalStatus; ///< Auto-calibration status
    std::vector<CalibrationColor> autoCalSequence; ///< Auto-calibration color sequence

    /**
     * @brief Find calibration point by target color
     * @param targetR Target red value
     * @param targetG Target green value
     * @param targetB Target blue value
     * @return Pointer to calibration point or nullptr if not found
     */
    const CalibrationPoint* findPointByTarget(uint8_t targetR, uint8_t targetG, uint8_t targetB) const;
    
    /**
     * @brief Recalculate the Color Correction Matrix
     * @return true if successful, false otherwise
     */
    bool recalculateCCM();
    
    /**
     * @brief Get target color by name
     * @param colorName Color name
     * @param r Output red value
     * @param g Output green value
     * @param b Output blue value
     * @return true if color found, false otherwise
     */
    bool getTargetColor(const String& colorName, uint8_t& r, uint8_t& g, uint8_t& b);
    


private:
    /**
     * @brief Perform dark offset calibration (LED OFF)
     * @return true if successful, false otherwise
     */
    bool performDarkOffsetCalibration();

    /**
     * @brief Perform black reference calibration (LED ON with black sample)
     * @return true if successful, false otherwise
     */
    bool performBlackReferenceCalibration();

    /**
     * @brief Perform normal calibration for non-black colors
     * @return true if successful, false otherwise
     */
    bool performNormalCalibration();

    /**
     * @brief Simple fallback conversion for when calibration is not available
     * @param rawX Raw X value
     * @param rawY Raw Y value
     * @param rawZ Raw Z value
     * @param r Output red value
     * @param g Output green value
     * @param b Output blue value
     */
    void normalizeXYZ(uint16_t rawX, uint16_t rawY, uint16_t rawZ, uint8_t& r, uint8_t& g, uint8_t& b);
};

#endif // COLOR_CALIBRATION_MANAGER_H
