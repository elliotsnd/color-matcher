/**
 * @file CIEDE2000.h
 * @brief Professional CIEDE2000 color difference calculation engine
 * 
 * This class implements the CIEDE2000 color difference formula, which is the
 * current industry standard for perceptually uniform color difference measurement.
 * CIEDE2000 provides more accurate color difference calculations than older
 * methods like CIE76 ΔE*ab, especially for saturated colors.
 * 
 * Key Features:
 * - Complete CIEDE2000 implementation with all correction terms
 * - LAB to LCH color space conversions
 * - RGB to LAB color space conversions (sRGB and Adobe RGB)
 * - Batch color difference calculations
 * - Quality assessment based on industry standards
 * - Optimized calculations for embedded systems
 * 
 * Industry Standards:
 * - ΔE < 1.0: Not perceptible by human eyes
 * - ΔE < 2.0: Perceptible through close observation
 * - ΔE < 3.0: Perceptible at a glance
 * - ΔE < 5.0: Clear difference, acceptable for some applications
 * - ΔE > 5.0: Significant difference, generally unacceptable
 * 
 * @author Enhanced Color Calibration System
 * @version 2.0
 * @date 2025-01-24
 */

#ifndef CIEDE2000_H
#define CIEDE2000_H

#include "Arduino.h"
#include <math.h>

/**
 * @brief LAB color space representation
 */
struct LABColor {
    float L;    // Lightness (0-100)
    float a;    // Green-Red axis (-128 to +127)
    float b;    // Blue-Yellow axis (-128 to +127)
    
    LABColor() : L(0), a(0), b(0) {}
    LABColor(float lightness, float aAxis, float bAxis) : L(lightness), a(aAxis), b(bAxis) {}
    
    String toString() const {
        return "LAB(" + String(L, 2) + ", " + String(a, 2) + ", " + String(b, 2) + ")";
    }
};

/**
 * @brief LCH color space representation (cylindrical LAB)
 */
struct LCHColor {
    float L;    // Lightness (0-100)
    float C;    // Chroma (0-100+)
    float H;    // Hue angle (0-360 degrees)
    
    LCHColor() : L(0), C(0), H(0) {}
    LCHColor(float lightness, float chroma, float hue) : L(lightness), C(chroma), H(hue) {}
    
    String toString() const {
        return "LCH(" + String(L, 2) + ", " + String(C, 2) + ", " + String(H, 1) + "°)";
    }
};

/**
 * @brief RGB color representation
 */
struct RGBColor {
    uint8_t R, G, B;
    
    RGBColor() : R(0), G(0), B(0) {}
    RGBColor(uint8_t red, uint8_t green, uint8_t blue) : R(red), G(green), B(blue) {}
    
    String toString() const {
        return "RGB(" + String(R) + ", " + String(G) + ", " + String(B) + ")";
    }
};

/**
 * @brief XYZ color space representation
 */
struct XYZColor {
    float X, Y, Z;
    
    XYZColor() : X(0), Y(0), Z(0) {}
    XYZColor(float x, float y, float z) : X(x), Y(y), Z(z) {}
    
    String toString() const {
        return "XYZ(" + String(X, 3) + ", " + String(Y, 3) + ", " + String(Z, 3) + ")";
    }
};

/**
 * @brief CIEDE2000 color difference result
 */
struct ColorDifferenceResult {
    float deltaE2000;          // CIEDE2000 color difference
    float deltaL;              // Lightness difference
    float deltaC;              // Chroma difference  
    float deltaH;              // Hue difference
    float deltaE76;            // CIE76 ΔE*ab for comparison
    bool acceptable;           // Whether difference is acceptable
    String qualityLevel;       // Quality assessment
    
    ColorDifferenceResult() : deltaE2000(0), deltaL(0), deltaC(0), deltaH(0), 
                             deltaE76(0), acceptable(false), qualityLevel("Unknown") {}
};

/**
 * @brief Color space conversion and CIEDE2000 calculation engine
 */
class CIEDE2000 {
private:
    // Standard illuminant D65 white point
    static constexpr float D65_X = 95.047f;
    static constexpr float D65_Y = 100.000f;
    static constexpr float D65_Z = 108.883f;
    
    // sRGB to XYZ conversion matrix
    static constexpr float sRGB_to_XYZ[3][3] = {
        {0.4124564f, 0.3575761f, 0.1804375f},
        {0.2126729f, 0.7151522f, 0.0721750f},
        {0.0193339f, 0.1191920f, 0.9503041f}
    };
    
    // Quality thresholds for different applications
    struct QualityThresholds {
        float excellent = 1.0f;     // ΔE < 1.0: Excellent quality
        float good = 2.0f;          // ΔE < 2.0: Good quality
        float acceptable = 3.0f;    // ΔE < 3.0: Acceptable quality
        float poor = 5.0f;          // ΔE < 5.0: Poor quality
        // ΔE > 5.0: Unacceptable quality
    } thresholds;
    
    /**
     * @brief Apply gamma correction for sRGB
     */
    float applySRGBGamma(float value) const;
    
    /**
     * @brief Remove gamma correction for sRGB
     */
    float removeSRGBGamma(float value) const;
    
    /**
     * @brief XYZ to LAB conversion helper
     */
    float xyzToLabHelper(float t) const;
    
    /**
     * @brief LAB to XYZ conversion helper
     */
    float labToXyzHelper(float t) const;
    
    /**
     * @brief Calculate hue angle with proper quadrant handling
     */
    float calculateHueAngle(float a, float b) const;
    
    /**
     * @brief Calculate hue difference with proper handling of circular nature
     */
    float calculateHueDifference(float h1, float h2) const;
    
    /**
     * @brief CIEDE2000 calculation helper functions
     */
    float calculateRT(float deltaTheta) const;
    float calculateSL(float L) const;
    float calculateSC(float C) const;
    float calculateSH(float C, float H) const;
    
public:
    /**
     * @brief Constructor
     */
    CIEDE2000();
    
    /**
     * @brief Convert RGB to XYZ color space
     * @param rgb RGB color (0-255 range)
     * @param colorSpace Color space ("sRGB" or "AdobeRGB")
     * @return XYZ color (0-100 range)
     */
    XYZColor rgbToXYZ(const RGBColor& rgb, const String& colorSpace = "sRGB") const;
    
    /**
     * @brief Convert XYZ to LAB color space
     * @param xyz XYZ color
     * @param illuminant Illuminant ("D65", "D50", etc.)
     * @return LAB color
     */
    LABColor xyzToLAB(const XYZColor& xyz, const String& illuminant = "D65") const;
    
    /**
     * @brief Convert LAB to LCH color space
     * @param lab LAB color
     * @return LCH color
     */
    LCHColor labToLCH(const LABColor& lab) const;
    
    /**
     * @brief Convert RGB directly to LAB
     * @param rgb RGB color
     * @param colorSpace Color space ("sRGB" or "AdobeRGB")
     * @return LAB color
     */
    LABColor rgbToLAB(const RGBColor& rgb, const String& colorSpace = "sRGB") const;
    
    /**
     * @brief Calculate CIEDE2000 color difference
     * @param lab1 First LAB color
     * @param lab2 Second LAB color
     * @return Complete color difference result
     */
    ColorDifferenceResult calculateDeltaE2000(const LABColor& lab1, const LABColor& lab2) const;
    
    /**
     * @brief Calculate CIEDE2000 color difference from RGB colors
     * @param rgb1 First RGB color
     * @param rgb2 Second RGB color
     * @param colorSpace Color space for conversion
     * @return Complete color difference result
     */
    ColorDifferenceResult calculateDeltaE2000(const RGBColor& rgb1, const RGBColor& rgb2, 
                                             const String& colorSpace = "sRGB") const;
    
    /**
     * @brief Calculate CIE76 ΔE*ab for comparison
     * @param lab1 First LAB color
     * @param lab2 Second LAB color
     * @return CIE76 color difference
     */
    float calculateDeltaE76(const LABColor& lab1, const LABColor& lab2) const;
    
    /**
     * @brief Assess color quality based on CIEDE2000 difference
     * @param deltaE CIEDE2000 color difference
     * @return Quality level string
     */
    String assessColorQuality(float deltaE) const;
    
    /**
     * @brief Check if color difference is acceptable for given application
     * @param deltaE CIEDE2000 color difference
     * @param application Application type ("printing", "display", "critical")
     * @return true if acceptable
     */
    bool isAcceptableForApplication(float deltaE, const String& application) const;
    
    /**
     * @brief Calculate batch color differences
     * @param referenceColors Array of reference LAB colors
     * @param measuredColors Array of measured LAB colors
     * @param count Number of color pairs
     * @param results Array to store results (must be pre-allocated)
     * @return Average CIEDE2000 difference
     */
    float calculateBatchDifferences(const LABColor* referenceColors, const LABColor* measuredColors,
                                   int count, ColorDifferenceResult* results) const;
    
    /**
     * @brief Update quality thresholds for specific application
     * @param excellent Threshold for excellent quality
     * @param good Threshold for good quality
     * @param acceptable Threshold for acceptable quality
     * @param poor Threshold for poor quality
     */
    void updateQualityThresholds(float excellent, float good, float acceptable, float poor);
    
    /**
     * @brief Get current quality thresholds
     */
    void getQualityThresholds(float& excellent, float& good, float& acceptable, float& poor) const;
    
    /**
     * @brief Validate LAB color values
     * @param lab LAB color to validate
     * @return true if values are within valid ranges
     */
    bool validateLABColor(const LABColor& lab) const;
    
    /**
     * @brief Get detailed color difference analysis
     * @param result Color difference result
     * @return Detailed analysis string
     */
    String getDetailedAnalysis(const ColorDifferenceResult& result) const;
    
    /**
     * @brief Calculate color gamut coverage
     * @param colors Array of LAB colors
     * @param count Number of colors
     * @return Gamut coverage percentage (0-100)
     */
    float calculateGamutCoverage(const LABColor* colors, int count) const;
};

#endif // CIEDE2000_H
