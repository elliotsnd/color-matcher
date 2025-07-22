/**
 * @file ColorScience.h
 * @brief Advanced color science library for accurate XYZ to RGB conversion
 * 
 * This library implements proper color space transformations using:
 * - Standard sRGB conversion matrices (ITU-R BT.709)
 * - Gamma correction (sRGB transfer function)
 * - White point normalization (D65 illuminant)
 * - Adaptive IR compensation
 * - Ambient light compensation
 * 
 * @author Color Calibration System
 * @version 1.0
 * @date 2025-07-21
 */

#ifndef COLOR_SCIENCE_H
#define COLOR_SCIENCE_H

#include <Arduino.h>
#include <math.h>

class ColorScience {
public:
    // Standard color space matrices and constants
    
    // sRGB conversion matrix (XYZ to linear RGB) - ITU-R BT.709 primaries, D65 white point
 static constexpr float XYZ_TO_S_RGB_MATRIX[9] = {
     3.2406f,  -1.5372f, -0.4986f,  // R = 3.2406*X - 1.5372*Y - 0.4986*Z
     -0.9689f, 1.8758f,  0.0415f,   // G = -0.9689*X + 1.8758*Y + 0.0415*Z
     0.0557f,  -0.2040f, 1.0570f    // B = 0.0557*X - 0.2040*Y + 1.0570*Z
 };

 // Alternative matrices for different color spaces
 static constexpr float XYZ_TO_ADOBE_RGB_MATRIX[9] = {
     2.0413f, -0.5649f, -0.3447f, -0.9692f, 1.8760f, 0.0416f, 0.0134f, -0.1184f, 1.0154f};

 // D65 white point (normalized)
 static constexpr float D65_WHITE_POINT[3] = {0.95047f, 1.00000f, 1.08883f};

 // sRGB gamma correction constants
 static constexpr float GAMMA_THRESHOLD = 0.0031308f;
 static constexpr float GAMMA_LINEAR_COEFF = 12.92f;
 static constexpr float GAMMA_POWER = 1.0f / 2.4f;
 static constexpr float GAMMA_OFFSET_A = 0.055f;
 static constexpr float GAMMA_SCALE = 1.055f;

 struct RGBColor {
   float r, g, b;
   uint8_t r8, g8, b8;  // 8-bit values
 };
    
    struct XYZColor {
        float X, Y, Z;
    };
    
    struct IRData {
        float IR1, IR2;
        float ratio;  // IR1/IR2 ratio for adaptive compensation
        float totalIR;  // Combined IR level
        float irTemperature;  // Estimated IR color temperature
        float ambientIRLevel;  // Ambient IR contamination level
    };
    
    struct CalibrationData {
        // White and black reference points
        XYZColor whiteReference;
        XYZColor blackReference;
        IRData whiteIR;
        IRData blackIR;

        // Dynamic IR compensation parameters
        float irCompensationFactor;  // Base IR compensation strength (0.0-1.0)
        bool ambientCompensationEnabled;

        // LED-specific IR compensation (simplified for controlled LED environment)
        struct {
            float baseIRCompensation;     // Base IR compensation for LED (0.0-0.3)
            float ledBrightnessResponse;  // How IR changes with LED brightness (0.0-0.1)
            float irToLedRatio;          // Expected IR/LED intensity ratio
            bool adaptToLEDBrightness;   // Adjust compensation based on LED level
            float minCompensation;       // Minimum IR compensation
            float maxCompensation;       // Maximum IR compensation
        } ledIR;

        // Spectral response for LED lighting (channel-specific IR leakage)
        struct {
            float xChannelIRLeakage;     // X channel IR contamination (typically 0.02-0.05)
            float yChannelIRLeakage;     // Y channel IR contamination (typically 0.01-0.03)
            float zChannelIRLeakage;     // Z channel IR contamination (typically 0.05-0.15)
            bool useChannelSpecificIR;   // Enable per-channel IR compensation
        } spectral;

        // Matrix selection
        bool useAdobeRGB;  // false = sRGB, true = Adobe RGB

        // Custom matrix override (if needed)
        bool useCustomMatrix;
        float customMatrix[9];
    };
    
    ColorScience();
    
    /**
     * @brief Convert XYZ to sRGB with proper color science
     * @param xyz Input XYZ color (normalized 0-1)
     * @param irData IR channel data for adaptive compensation
     * @param calibData Calibration data including references
     * @return RGB color with both float and 8-bit values
     */
    static RGBColor xyzToRGB(const XYZColor& xyz, const IRData& irData, const CalibrationData& calibData);
    
    /**
     * @brief Convert raw sensor values to normalized XYZ
     * @param rawX Raw X channel value
     * @param rawY Raw Y channel value  
     * @param rawZ Raw Z channel value
     * @param rawIR1 Raw IR1 channel value
     * @param rawIR2 Raw IR2 channel value
     * @param calibData Calibration data
     * @return Normalized XYZ color
     */
    static XYZColor rawToXYZ(uint16_t rawX, uint16_t rawY, uint16_t rawZ, 
                            uint16_t rawIR1, uint16_t rawIR2, 
                            const CalibrationData& calibData);
    
    /**
     * @brief Apply sRGB gamma correction
     * @param linear Linear RGB value (0-1)
     * @return Gamma-corrected value (0-1)
     */
    static float applySRGBGamma(float linear);
    
    /**
     * @brief Apply inverse sRGB gamma correction
     * @param gamma Gamma-corrected value (0-1)
     * @return Linear RGB value (0-1)
     */
    static float applyInverseSRGBGamma(float gamma);
    
    /**
     * @brief Normalize XYZ values using white point
     * @param xyz Input XYZ color
     * @param whitePoint White point reference
     * @return Normalized XYZ color
     */
    static XYZColor normalizeXYZ(const XYZColor& xyz, const XYZColor& whitePoint);
    
    /**
     * @brief Apply adaptive IR compensation
     * @param xyz Input XYZ color
     * @param irData IR channel data
     * @param calibData Calibration data
     * @return IR-compensated XYZ color
     */
    static XYZColor applyIRCompensation(const XYZColor& xyz, const IRData& irData, 
                                       const CalibrationData& calibData);
    
    /**
     * @brief Apply ambient light compensation (black subtraction)
     * @param xyz Input XYZ color
     * @param calibData Calibration data with black reference
     * @return Ambient-compensated XYZ color
     */
    static XYZColor applyAmbientCompensation(const XYZColor& xyz, const CalibrationData& calibData);
    
    /**
     * @brief Matrix multiplication for color space conversion
     * @param matrix 3x3 conversion matrix (row-major)
     * @param input Input color vector [X, Y, Z]
     * @param output Output color vector [R, G, B]
     */
    static void matrixMultiply3x3(const float matrix[9], const float input[3], float output[3]);
    
    /**
     * @brief Clamp value to valid range
     * @param value Input value
     * @param min Minimum value
     * @param max Maximum value
     * @return Clamped value
     */
    static float clamp(float value, float min = 0.0f, float max = 1.0f);
    
    /**
     * @brief Convert float RGB (0-1) to 8-bit RGB (0-255)
     * @param r Red component (0-1)
     * @param g Green component (0-1)
     * @param b Blue component (0-1)
     * @param r8 Output red (0-255)
     * @param g8 Output green (0-255)
     * @param b8 Output blue (0-255)
     */
    static void floatToRGB8(float r, float g, float b, uint8_t& r8, uint8_t& g8, uint8_t& b8);
    
    /**
     * @brief Calculate color temperature from XYZ (McCamy's formula)
     * @param xyz Input XYZ color
     * @return Color temperature in Kelvin
     */
    static float calculateColorTemperature(const XYZColor& xyz);
    
    /**
     * @brief Calculate chromaticity coordinates
     * @param xyz Input XYZ color
     * @param x Output x chromaticity
     * @param y Output y chromaticity
     */
    static void calculateChromaticity(const XYZColor& xyz, float& x, float& y);
    
    /**
     * @brief Validate and initialize calibration data
     * @param calibData Calibration data to validate
     * @return true if valid, false if needs recalibration
     */
    static bool validateCalibrationData(const CalibrationData& calibData);
    
    /**
     * @brief Create default calibration data
     * @return Default calibration data structure
     */
    static CalibrationData createDefaultCalibration();
    
private:
    // Helper functions for advanced color science
    static float linearInterpolate(float x, float x1, float y1, float x2, float y2);
    static float calculateIRContamination(const IRData& irData);
    static XYZColor applyWhiteBalance(const XYZColor& xyz, const XYZColor& whitePoint);
};

#endif // COLOR_SCIENCE_H
