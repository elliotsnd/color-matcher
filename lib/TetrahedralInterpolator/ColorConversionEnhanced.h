/**
 * @file ColorConversionEnhanced.h
 * @brief Enhanced color conversion system with 4-point tetrahedral interpolation
 * 
 * This module provides the enhanced color conversion functions that integrate
 * the tetrahedral interpolation engine with the existing calibration system.
 * It maintains backward compatibility while providing superior accuracy.
 * 
 * @author Enhanced Color Calibration System
 * @version 2.0
 * @date 2025-01-24
 */

#ifndef COLOR_CONVERSION_ENHANCED_H
#define COLOR_CONVERSION_ENHANCED_H

#include "Arduino.h"
#include "TetrahedralInterpolator.h"
#include "ColorScience.h"

/**
 * @brief Enhanced color conversion manager
 */
class ColorConversionEnhanced {
private:
    TetrahedralInterpolator tetrahedralInterpolator;
    bool isTetrahedralReady = false;
    
    // Performance tracking
    uint32_t total2PointConversions = 0;
    uint32_t total4PointConversions = 0;
    uint32_t totalFallbackConversions = 0;
    
    // IR compensation (from existing system)
    float applyIRCompensation(uint16_t rawValue, uint16_t ir1, uint16_t ir2, 
                             const ColorScience::CalibrationData& calibData) const;
    
    // Legacy 2-point conversion (backward compatibility)
    void convertXyZtoRgbLegacy(uint16_t X, uint16_t Y, uint16_t Z, uint16_t IR1, uint16_t IR2,
                              uint8_t &R, uint8_t &G, uint8_t &B,
                              const ColorScience::CalibrationData& calibData);
    
public:
    /**
     * @brief Constructor
     */
    ColorConversionEnhanced();
    
    /**
     * @brief Initialize the enhanced conversion system
     * @param calibData Complete calibration data
     * @return true if initialization successful
     */
    bool initialize(const ColorScience::CalibrationData& calibData);
    
    /**
     * @brief Enhanced XYZ to RGB conversion with automatic method selection
     * @param X Raw X sensor value
     * @param Y Raw Y sensor value
     * @param Z Raw Z sensor value
     * @param IR1 Raw IR1 sensor value
     * @param IR2 Raw IR2 sensor value
     * @param R Output red value (0-255)
     * @param G Output green value (0-255)
     * @param B Output blue value (0-255)
     * @param calibData Calibration data
     * @return Conversion method used (0=fallback, 1=2-point, 2=4-point)
     */
    int convertXyZtoRgbEnhanced(uint16_t X, uint16_t Y, uint16_t Z, uint16_t IR1, uint16_t IR2,
                               uint8_t &R, uint8_t &G, uint8_t &B,
                               const ColorScience::CalibrationData& calibData);
    
    /**
     * @brief Force 4-point tetrahedral conversion (for testing)
     * @param X Raw X sensor value
     * @param Y Raw Y sensor value
     * @param Z Raw Z sensor value
     * @param IR1 Raw IR1 sensor value
     * @param IR2 Raw IR2 sensor value
     * @param R Output red value (0-255)
     * @param G Output green value (0-255)
     * @param B Output blue value (0-255)
     * @param calibData Calibration data
     * @return true if conversion successful
     */
    bool convertXyZtoRgb4Point(uint16_t X, uint16_t Y, uint16_t Z, uint16_t IR1, uint16_t IR2,
                              uint8_t &R, uint8_t &G, uint8_t &B,
                              const ColorScience::CalibrationData& calibData);
    
    /**
     * @brief Get conversion performance statistics
     */
    void getConversionStatistics(uint32_t& total2Point, uint32_t& total4Point, 
                                uint32_t& totalFallback, float& accuracy4Point) const;
    
    /**
     * @brief Reset performance counters
     */
    void resetStatistics();
    
    /**
     * @brief Get detailed debug information
     */
    String getDebugInfo() const;
    
    /**
     * @brief Test conversion accuracy with known test points
     * @param calibData Calibration data to test
     * @return Average color error across test points
     */
    float testConversionAccuracy(const ColorScience::CalibrationData& calibData);
    
    /**
     * @brief Check if tetrahedral interpolation is available
     */
    bool isTetrahedralAvailable() const { return isTetrahedralReady; }
    
    /**
     * @brief Reinitialize tetrahedral interpolator (after calibration update)
     */
    bool reinitialize(const ColorScience::CalibrationData& calibData);
};

// Global instance for easy integration
extern ColorConversionEnhanced colorConverter;

/**
 * @brief Enhanced color conversion function (drop-in replacement)
 * 
 * This function provides a drop-in replacement for the existing convertXyZtoRgbVividWhite
 * function while adding 4-point tetrahedral interpolation capabilities.
 * 
 * @param X Raw X sensor value
 * @param Y Raw Y sensor value
 * @param Z Raw Z sensor value
 * @param IR1 Raw IR1 sensor value
 * @param IR2 Raw IR2 sensor value
 * @param R Output red value (0-255)
 * @param G Output green value (0-255)
 * @param B Output blue value (0-255)
 * @param calibData Calibration data (optional, uses global if not provided)
 */
void convertXyZtoRgbMultiPoint(uint16_t X, uint16_t Y, uint16_t Z, uint16_t IR1, uint16_t IR2,
                              uint8_t &R, uint8_t &G, uint8_t &B,
                              const ColorScience::CalibrationData* calibData = nullptr);

/**
 * @brief Test color conversion accuracy
 * @param calibData Calibration data to test
 * @return Test results as JSON string
 */
String testColorConversionAccuracy(const ColorScience::CalibrationData& calibData);

/**
 * @brief Benchmark conversion performance
 * @param iterations Number of test iterations
 * @return Performance results as JSON string
 */
String benchmarkConversionPerformance(int iterations = 1000);

#endif // COLOR_CONVERSION_ENHANCED_H
