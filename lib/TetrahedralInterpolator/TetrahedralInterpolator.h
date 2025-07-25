/**
 * @file TetrahedralInterpolator.h
 * @brief Robust tetrahedral interpolation for 4-point color calibration
 * 
 * This class implements mathematically sound tetrahedral interpolation for
 * converting sensor XYZ values to RGB using four calibration reference points:
 * Black, White, Blue, and Yellow.
 * 
 * Key Features:
 * - Barycentric coordinate calculation using Cramer's rule
 * - Robust fallback mechanisms for degenerate cases
 * - Distance-weighted interpolation for out-of-gamut colors
 * - Performance optimized for ESP32 platforms
 * 
 * @author Enhanced Color Calibration System
 * @version 2.0
 * @date 2025-01-24
 */

#ifndef TETRAHEDRAL_INTERPOLATOR_H
#define TETRAHEDRAL_INTERPOLATOR_H

#include "Arduino.h"
#include "ColorScience.h"

/**
 * @brief Tetrahedral interpolation weights for 4-point calibration
 */
struct TetrahedralWeights {
    float black;     // Weight for black reference point
    float white;     // Weight for white reference point  
    float blue;      // Weight for blue reference point
    float yellow;    // Weight for yellow reference point
    bool isValid;    // Whether interpolation was successful
    
    TetrahedralWeights() : black(0), white(0), blue(0), yellow(0), isValid(false) {}
    
    /**
     * @brief Normalize weights to sum to 1.0
     */
    void normalize() {
        float sum = black + white + blue + yellow;
        if (sum > 0.001f) {
            black /= sum;
            white /= sum;
            blue /= sum;
            yellow /= sum;
            isValid = true;
        }
    }
    
    /**
     * @brief Check if all weights are non-negative (point inside tetrahedron)
     */
    bool isInsideTetrahedron() const {
        return (black >= -0.001f && white >= -0.001f && blue >= -0.001f && yellow >= -0.001f);
    }
    
    /**
     * @brief Get debug string representation
     */
    String toString() const {
        return "B:" + String(black, 3) + " W:" + String(white, 3) + 
               " Bl:" + String(blue, 3) + " Y:" + String(yellow, 3) + 
               " Valid:" + String(isValid ? "T" : "F");
    }
};

/**
 * @brief 3D point structure for geometric calculations
 */
struct Point3D {
    float x, y, z;
    
    Point3D(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}
    
    /**
     * @brief Calculate Euclidean distance to another point
     */
    float distanceTo(const Point3D& other) const {
        return sqrt(pow(x - other.x, 2) + pow(y - other.y, 2) + pow(z - other.z, 2));
    }
    
    /**
     * @brief Vector subtraction
     */
    Point3D operator-(const Point3D& other) const {
        return Point3D(x - other.x, y - other.y, z - other.z);
    }
    
    /**
     * @brief Get debug string representation
     */
    String toString() const {
        return "(" + String(x, 3) + "," + String(y, 3) + "," + String(z, 3) + ")";
    }
};

/**
 * @brief RGB color structure for interpolation results
 */
struct RGBColor {
    float r, g, b;
    
    RGBColor(float r = 0, float g = 0, float b = 0) : r(r), g(g), b(b) {}
    
    /**
     * @brief Convert to 8-bit RGB values with bounds checking
     */
    void to8Bit(uint8_t& r8, uint8_t& g8, uint8_t& b8) const {
        r8 = static_cast<uint8_t>(constrain(r, 0, 255));
        g8 = static_cast<uint8_t>(constrain(g, 0, 255));
        b8 = static_cast<uint8_t>(constrain(b, 0, 255));
    }
};

/**
 * @brief Tetrahedral interpolation engine for 4-point color calibration
 */
class TetrahedralInterpolator {
private:
    // Reference points in normalized XYZ space [0,1]
    Point3D blackPoint, whitePoint, bluePoint, yellowPoint;
    
    // Target RGB values for each reference point
    RGBColor blackRGB, whiteRGB, blueRGB, yellowRGB;
    
    // Initialization and validation flags
    bool isInitialized = false;
    bool isValidTetrahedron = false;
    
    // Performance and debugging
    uint32_t interpolationCount = 0;
    uint32_t fallbackCount = 0;
    
    /**
     * @brief Calculate 3x3 matrix determinant
     */
    float calculateDeterminant3x3(const float matrix[3][3]) const;
    
    /**
     * @brief Calculate determinant with column replacement (Cramer's rule)
     */
    float calculateDeterminantWithColumn(const float matrix[3][3], const float rhs[3], int col) const;
    
    /**
     * @brief Calculate barycentric coordinates using Cramer's rule
     */
    TetrahedralWeights calculateBarycentricWeights(const Point3D& p) const;
    
    /**
     * @brief Fallback to triangular interpolation for degenerate cases
     */
    TetrahedralWeights calculateTriangularFallback(const Point3D& p) const;
    
    /**
     * @brief Distance-weighted interpolation for out-of-gamut points
     */
    TetrahedralWeights calculateDistanceWeightedFallback(const Point3D& p) const;
    
    /**
     * @brief Calculate triangle weights using barycentric coordinates
     */
    TetrahedralWeights calculateTriangleWeights(const Point3D& p, const Point3D& p1, 
                                               const Point3D& p2, const Point3D& p3) const;
    
    /**
     * @brief Validate tetrahedron geometry
     */
    bool validateTetrahedronGeometry();
    
public:
    /**
     * @brief Constructor
     */
    TetrahedralInterpolator();
    
    /**
     * @brief Initialize interpolator with calibration data
     * @param calibData Complete calibration data with all 4 reference points
     * @return true if initialization successful
     */
    bool initialize(const ColorScience::CalibrationData& calibData);
    
    /**
     * @brief Perform tetrahedral interpolation
     * @param X Raw X sensor value
     * @param Y Raw Y sensor value  
     * @param Z Raw Z sensor value
     * @return Interpolation weights for the 4 reference points
     */
    TetrahedralWeights interpolate(uint16_t X, uint16_t Y, uint16_t Z);
    
    /**
     * @brief Convert XYZ to RGB using tetrahedral interpolation
     * @param X Raw X sensor value
     * @param Y Raw Y sensor value
     * @param Z Raw Z sensor value
     * @param R Output red value (0-255)
     * @param G Output green value (0-255)
     * @param B Output blue value (0-255)
     * @return true if conversion successful
     */
    bool convertXYZtoRGB(uint16_t X, uint16_t Y, uint16_t Z, uint8_t& R, uint8_t& G, uint8_t& B);
    
    /**
     * @brief Check if interpolator is properly initialized
     */
    bool isReady() const { return isInitialized && isValidTetrahedron; }
    
    /**
     * @brief Get performance statistics
     */
    void getStatistics(uint32_t& totalInterpolations, uint32_t& fallbackUsed, float& fallbackRate) const;
    
    /**
     * @brief Reset performance counters
     */
    void resetStatistics();
    
    /**
     * @brief Get debug information about reference points
     */
    String getDebugInfo() const;
    
    /**
     * @brief Validate interpolation with test point
     * @param testX Test X coordinate
     * @param testY Test Y coordinate  
     * @param testZ Test Z coordinate
     * @param expectedR Expected red output
     * @param expectedG Expected green output
     * @param expectedB Expected blue output
     * @return Color difference (CIEDE2000-like metric)
     */
    float validateInterpolation(uint16_t testX, uint16_t testY, uint16_t testZ,
                               uint8_t expectedR, uint8_t expectedG, uint8_t expectedB);
};

#endif // TETRAHEDRAL_INTERPOLATOR_H
