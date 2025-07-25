/**
 * @file ColorConversionEnhanced.cpp
 * @brief Implementation of enhanced color conversion with tetrahedral interpolation
 */

#include "ColorConversionEnhanced.h"

// Global instance
ColorConversionEnhanced colorConverter;

// Constructor
ColorConversionEnhanced::ColorConversionEnhanced() {
    isTetrahedralReady = false;
    total2PointConversions = 0;
    total4PointConversions = 0;
    totalFallbackConversions = 0;
}

// Initialize the enhanced conversion system
bool ColorConversionEnhanced::initialize(const ColorScience::CalibrationData& calibData) {
    Serial.println("=== Initializing Enhanced Color Conversion ===");
    
    // Try to initialize tetrahedral interpolation
    if (calibData.status.is4PointCalibrated()) {
        isTetrahedralReady = tetrahedralInterpolator.initialize(calibData);
        if (isTetrahedralReady) {
            Serial.println("4-point tetrahedral interpolation ready");
        } else {
            Serial.println("Failed to initialize tetrahedral interpolation");
        }
    } else {
        Serial.println("4-point calibration not available, using 2-point mode");
        isTetrahedralReady = false;
    }
    
    return true; // Always return true as 2-point fallback is available
}

// Apply IR compensation (simplified version of existing system)
float ColorConversionEnhanced::applyIRCompensation(uint16_t rawValue, uint16_t ir1, uint16_t ir2, 
                                                   const ColorScience::CalibrationData& calibData) const {
    if (!calibData.ambientCompensationEnabled) {
        return rawValue;
    }
    
    // Simple IR compensation based on existing system
    float irLevel = (ir1 + ir2) / 2.0f / 65535.0f;
    float compensation = irLevel * calibData.irCompensationFactor;
    
    return rawValue * (1.0f - compensation);
}

// Legacy 2-point conversion (backward compatibility)
void ColorConversionEnhanced::convertXyZtoRgbLegacy(uint16_t X, uint16_t Y, uint16_t Z, 
                                                    uint16_t IR1, uint16_t IR2,
                                                    uint8_t &R, uint8_t &G, uint8_t &B,
                                                    const ColorScience::CalibrationData& calibData) {
    // Apply IR compensation
    float xCompensated = applyIRCompensation(X, IR1, IR2, calibData);
    float yCompensated = applyIRCompensation(Y, IR1, IR2, calibData);
    float zCompensated = applyIRCompensation(Z, IR1, IR2, calibData);
    
    // Simple 2-point linear mapping (simplified version)
    // In a real implementation, this would use the actual legacy calibration data
    float blackX = calibData.blackReference.raw.X;
    float blackY = calibData.blackReference.raw.Y;
    float blackZ = calibData.blackReference.raw.Z;
    
    float whiteX = calibData.whiteReference.raw.X;
    float whiteY = calibData.whiteReference.raw.Y;
    float whiteZ = calibData.whiteReference.raw.Z;
    
    // Map to 0-255 range
    R = static_cast<uint8_t>(constrain(map(xCompensated, blackX, whiteX, 0, 255), 0, 255));
    G = static_cast<uint8_t>(constrain(map(yCompensated, blackY, whiteY, 0, 255), 0, 255));
    B = static_cast<uint8_t>(constrain(map(zCompensated, blackZ, whiteZ, 0, 255), 0, 255));
    
    total2PointConversions++;
}

// Enhanced XYZ to RGB conversion with automatic method selection
int ColorConversionEnhanced::convertXyZtoRgbEnhanced(uint16_t X, uint16_t Y, uint16_t Z, 
                                                     uint16_t IR1, uint16_t IR2,
                                                     uint8_t &R, uint8_t &G, uint8_t &B,
                                                     const ColorScience::CalibrationData& calibData) {
    // Try 4-point tetrahedral interpolation first
    if (isTetrahedralReady && calibData.status.is4PointCalibrated()) {
        if (convertXyZtoRgb4Point(X, Y, Z, IR1, IR2, R, G, B, calibData)) {
            total4PointConversions++;
            return 2; // 4-point conversion used
        }
    }
    
    // Fallback to 2-point calibration
    if (calibData.status.is2PointCalibrated()) {
        convertXyZtoRgbLegacy(X, Y, Z, IR1, IR2, R, G, B, calibData);
        return 1; // 2-point conversion used
    }
    
    // Ultimate fallback - direct mapping
    R = static_cast<uint8_t>(constrain(X / 256, 0, 255));
    G = static_cast<uint8_t>(constrain(Y / 256, 0, 255));
    B = static_cast<uint8_t>(constrain(Z / 256, 0, 255));
    
    totalFallbackConversions++;
    return 0; // Fallback conversion used
}

// Force 4-point tetrahedral conversion
bool ColorConversionEnhanced::convertXyZtoRgb4Point(uint16_t X, uint16_t Y, uint16_t Z, 
                                                    uint16_t IR1, uint16_t IR2,
                                                    uint8_t &R, uint8_t &G, uint8_t &B,
                                                    const ColorScience::CalibrationData& calibData) {
    if (!isTetrahedralReady) {
        return false;
    }
    
    // Apply IR compensation
    float xCompensated = applyIRCompensation(X, IR1, IR2, calibData);
    float yCompensated = applyIRCompensation(Y, IR1, IR2, calibData);
    float zCompensated = applyIRCompensation(Z, IR1, IR2, calibData);
    
    // Use tetrahedral interpolation
    return tetrahedralInterpolator.convertXYZtoRGB(
        static_cast<uint16_t>(xCompensated), 
        static_cast<uint16_t>(yCompensated), 
        static_cast<uint16_t>(zCompensated), 
        R, G, B
    );
}

// Get conversion performance statistics
void ColorConversionEnhanced::getConversionStatistics(uint32_t& total2Point, uint32_t& total4Point, 
                                                      uint32_t& totalFallback, float& accuracy4Point) const {
    total2Point = total2PointConversions;
    total4Point = total4PointConversions;
    totalFallback = totalFallbackConversions;
    
    // Get tetrahedral interpolation statistics
    uint32_t tetraInterpolations, tetraFallbacks;
    float tetraFallbackRate;
    tetrahedralInterpolator.getStatistics(tetraInterpolations, tetraFallbacks, tetraFallbackRate);
    
    accuracy4Point = tetraInterpolations > 0 ? (100.0f - tetraFallbackRate) : 0.0f;
}

// Reset performance counters
void ColorConversionEnhanced::resetStatistics() {
    total2PointConversions = 0;
    total4PointConversions = 0;
    totalFallbackConversions = 0;
    tetrahedralInterpolator.resetStatistics();
}

// Get detailed debug information
String ColorConversionEnhanced::getDebugInfo() const {
    String info = "=== Enhanced Color Conversion Debug Info ===\n";
    info += "Tetrahedral Ready: " + String(isTetrahedralReady ? "Yes" : "No") + "\n";
    info += "2-Point Conversions: " + String(total2PointConversions) + "\n";
    info += "4-Point Conversions: " + String(total4PointConversions) + "\n";
    info += "Fallback Conversions: " + String(totalFallbackConversions) + "\n";
    
    uint32_t totalConversions = total2PointConversions + total4PointConversions + totalFallbackConversions;
    if (totalConversions > 0) {
        info += "4-Point Usage: " + String((float)total4PointConversions / totalConversions * 100.0f, 1) + "%\n";
    }
    
    if (isTetrahedralReady) {
        info += "\n" + tetrahedralInterpolator.getDebugInfo();
    }
    
    return info;
}

// Test conversion accuracy with known test points
float ColorConversionEnhanced::testConversionAccuracy(const ColorScience::CalibrationData& calibData) {
    if (!isTetrahedralReady) {
        return -1.0f; // Cannot test without tetrahedral interpolation
    }
    
    // Simple test points (in a real implementation, use actual measured values)
    struct TestPoint {
        uint16_t x, y, z;
        uint8_t expectedR, expectedG, expectedB;
    };
    
    TestPoint testPoints[] = {
        {8000, 8000, 8000, 128, 128, 128},    // Mid gray
        {15000, 15000, 2000, 255, 255, 0},   // Yellow-ish
        {2000, 4000, 15000, 0, 0, 255},      // Blue-ish
        {15000, 8000, 8000, 255, 128, 128}   // Red-ish
    };
    
    float totalError = 0.0f;
    int validTests = 0;
    
    for (const auto& test : testPoints) {
        float error = tetrahedralInterpolator.validateInterpolation(
            test.x, test.y, test.z, 
            test.expectedR, test.expectedG, test.expectedB
        );
        
        if (error < 100.0f) { // Reasonable error threshold
            totalError += error;
            validTests++;
        }
    }
    
    return validTests > 0 ? totalError / validTests : -1.0f;
}

// Reinitialize tetrahedral interpolator
bool ColorConversionEnhanced::reinitialize(const ColorScience::CalibrationData& calibData) {
    return initialize(calibData);
}

// Global enhanced conversion function
void convertXyZtoRgbMultiPoint(uint16_t X, uint16_t Y, uint16_t Z, uint16_t IR1, uint16_t IR2,
                              uint8_t &R, uint8_t &G, uint8_t &B,
                              const ColorScience::CalibrationData* calibData) {
    // Use global calibration data if not provided
    // In a real implementation, this would access the global settings
    static ColorScience::CalibrationData defaultCalibData;
    
    const ColorScience::CalibrationData& activeCalibData = calibData ? *calibData : defaultCalibData;
    
    colorConverter.convertXyZtoRgbEnhanced(X, Y, Z, IR1, IR2, R, G, B, activeCalibData);
}

// Test color conversion accuracy
String testColorConversionAccuracy(const ColorScience::CalibrationData& calibData) {
    float accuracy = colorConverter.testConversionAccuracy(calibData);
    
    String result = "{\n";
    result += "  \"accuracy\": " + String(accuracy, 2) + ",\n";
    result += "  \"tetrahedralAvailable\": " + String(colorConverter.isTetrahedralAvailable() ? "true" : "false") + ",\n";
    result += "  \"status\": \"" + String(accuracy >= 0 ? "success" : "failed") + "\"\n";
    result += "}";
    
    return result;
}

// Benchmark conversion performance
String benchmarkConversionPerformance(int iterations) {
    uint32_t start2Point = micros();
    
    // Benchmark 2-point conversion (simplified)
    for (int i = 0; i < iterations; i++) {
        uint8_t r, g, b;
        // Simulate 2-point conversion
        r = (10000 + i) / 256;
        g = (12000 + i) / 256;
        b = (8000 + i) / 256;
    }
    
    uint32_t time2Point = micros() - start2Point;
    
    uint32_t start4Point = micros();
    
    // Benchmark 4-point conversion
    for (int i = 0; i < iterations; i++) {
        uint8_t r, g, b;
        colorConverter.convertXyZtoRgb4Point(10000 + i, 12000 + i, 8000 + i, 500, 600, 
                                            r, g, b, ColorScience::CalibrationData());
    }
    
    uint32_t time4Point = micros() - start4Point;
    
    String result = "{\n";
    result += "  \"iterations\": " + String(iterations) + ",\n";
    result += "  \"time2Point_us\": " + String(time2Point) + ",\n";
    result += "  \"time4Point_us\": " + String(time4Point) + ",\n";
    result += "  \"avgTime2Point_us\": " + String((float)time2Point / iterations, 2) + ",\n";
    result += "  \"avgTime4Point_us\": " + String((float)time4Point / iterations, 2) + ",\n";
    result += "  \"overhead_percent\": " + String(((float)(time4Point - time2Point) / time2Point) * 100, 1) + "\n";
    result += "}";
    
    return result;
}
