/**
 * @file ColorCalibration.h
 * @brief Main interface for the 5-Point Color Correction Matrix calibration system
 * @author ESP32 Color Calibration System
 * @version 1.0.0
 * @date 2024
 * 
 * This file provides the main interface for the complete color calibration system.
 */

#ifndef COLOR_CALIBRATION_H
#define COLOR_CALIBRATION_H

#include "ColorCalibrationManager.h"
#include "CalibrationStructures.h"
#include <Arduino.h>

/**
 * @brief Main color calibration interface class
 * 
 * Provides a simplified interface to the color calibration system with
 * static methods for easy integration into existing projects.
 */
class ColorCalibration {
public:
    /**
     * @brief Initialize the color calibration system
     * @return true if initialization successful, false otherwise
     */
    static bool initialize();
    
    /**
     * @brief Convert raw sensor readings to calibrated RGB values
     * @param rawX Raw X sensor reading
     * @param rawY Raw Y sensor reading
     * @param rawZ Raw Z sensor reading
     * @param r Output red value (0-255)
     * @param g Output green value (0-255)
     * @param b Output blue value (0-255)
     * @return true if calibration applied, false if using fallback
     */
    static bool convertColor(uint16_t rawX, uint16_t rawY, uint16_t rawZ, uint8_t& r, uint8_t& g, uint8_t& b);
    
    /**
     * @brief Check if the system is fully calibrated
     * @return true if all 5 colors are calibrated
     */
    static bool isCalibrated();
    
    /**
     * @brief Get the current Color Correction Matrix
     * @return Color correction matrix
     */
    static ColorCorrectionMatrix getColorCorrectionMatrix();
    
    /**
     * @brief Reset all calibration data
     * @return true if successful, false otherwise
     */
    static bool resetCalibration();
    
    /**
     * @brief Get access to the underlying calibration manager
     * @return Reference to the ColorCalibrationManager instance
     */
    static ColorCalibrationManager& getManager();

private:
    static ColorCalibrationManager manager; ///< Static manager instance
};

// Convenience macros for drop-in replacement functionality
#define COLOR_CALIBRATION_INIT() ColorCalibration::initialize()
#define COLOR_CALIBRATION_CONVERT(x, y, z, r, g, b) ColorCalibration::convertColor(x, y, z, r, g, b)
#define COLOR_CALIBRATION_IS_READY() ColorCalibration::isCalibrated()
#define COLOR_CALIBRATION_RESET() ColorCalibration::resetCalibration()
#define COLOR_CALIBRATION_GET_MANAGER() ColorCalibration::getManager()

#endif // COLOR_CALIBRATION_H
