/**
 * @file SwatchTesting.h
 * @brief RGB color swatch testing framework for sensor validation
 * 
 * This framework provides:
 * - Standard color swatch definitions (Pantone, RAL, etc.)
 * - Comparison algorithms for sensor vs reference values
 * - Statistical analysis of color accuracy
 * - Delta E calculations for color difference
 * - Calibration quality assessment
 * 
 * @author Color Calibration System
 * @version 1.0
 * @date 2025-07-21
 */

#ifndef SWATCH_TESTING_H
#define SWATCH_TESTING_H

#include <Arduino.h>
#include "../ColorScience/ColorScience.h"

class SwatchTesting {
public:
    // Standard color swatch definitions
    struct ColorSwatch {
        const char* name;
        uint8_t referenceR, referenceG, referenceB;  // Expected RGB values
        float referenceX, referenceY, referenceZ;    // Expected XYZ values (if known)
        float tolerance;                             // Acceptable error tolerance
        bool hasXYZ;                                // Whether XYZ values are provided
    };
    
    // Test results for a single swatch
    struct SwatchResult {
        ColorSwatch swatch;
        uint8_t measuredR, measuredG, measuredB;
        float deltaE;                               // Color difference (Delta E)
        float rgbError;                            // RGB Euclidean distance
        bool passed;                               // Whether test passed tolerance
        float accuracy;                            // Accuracy percentage (0-100%)
    };
    
    // Overall test session results
    struct TestSession {
        SwatchResult results[32];                  // Up to 32 swatches per session
        uint8_t numSwatches;
        float averageDeltaE;
        float averageAccuracy;
        float worstDeltaE;
        float bestDeltaE;
        uint8_t passedCount;
        uint8_t failedCount;
        bool sessionPassed;                        // Overall session pass/fail
    };
    
    SwatchTesting();
    
    /**
     * @brief Initialize a new test session
     * @return true if successful
     */
    bool beginTestSession();
    
    /**
     * @brief Add a color swatch test to the current session
     * @param swatch The color swatch definition
     * @param measuredRGB The measured RGB values from sensor
     * @return SwatchResult with test results
     */
    SwatchResult testSwatch(const ColorSwatch& swatch, const ColorScience::RGBColor& measuredRGB);
    
    /**
     * @brief Finalize test session and calculate statistics
     * @return TestSession with complete results
     */
    TestSession finalizeTestSession();
    
    /**
     * @brief Calculate Delta E color difference (CIE76 formula)
     * @param lab1 First color in LAB space
     * @param lab2 Second color in LAB space
     * @return Delta E value
     */
    static float calculateDeltaE(const float lab1[3], const float lab2[3]);
    
    /**
     * @brief Convert RGB to LAB color space for Delta E calculation
     * @param r Red component (0-255)
     * @param g Green component (0-255)
     * @param b Blue component (0-255)
     * @param lab Output LAB values [L, a, b]
     */
    static void rgbToLab(uint8_t r, uint8_t g, uint8_t b, float lab[3]);
    
    /**
     * @brief Calculate RGB Euclidean distance
     * @param r1 First red component
     * @param g1 First green component
     * @param b1 First blue component
     * @param r2 Second red component
     * @param g2 Second green component
     * @param b2 Second blue component
     * @return Euclidean distance (0-441.67)
     */
    static float calculateRGBDistance(uint8_t r1, uint8_t g1, uint8_t b1, 
                                     uint8_t r2, uint8_t g2, uint8_t b2);
    
    /**
     * @brief Print test results to Serial
     * @param result The swatch test result to print
     */
    static void printSwatchResult(const SwatchResult& result);
    
    /**
     * @brief Print complete test session results
     * @param session The test session to print
     */
    static void printTestSession(const TestSession& session);
    
    /**
     * @brief Get predefined standard color swatches
     * @param swatchSet Which set to get (0=basic, 1=extended, 2=pantone)
     * @param count Output parameter for number of swatches
     * @return Pointer to swatch array
     */
    static const ColorSwatch* getStandardSwatches(uint8_t swatchSet, uint8_t& count);
    
    // Predefined color swatch sets
    
    // Basic RGB test colors (8 colors)
    static const ColorSwatch BASIC_RGB_SWATCHES[];
    static const uint8_t BASIC_RGB_COUNT = 8;
    
    // Extended color set (16 colors)
    static const ColorSwatch EXTENDED_SWATCHES[];
    static const uint8_t EXTENDED_COUNT = 16;
    
    // Pantone-like colors (12 colors)
    static const ColorSwatch PANTONE_LIKE_SWATCHES[];
    static const uint8_t PANTONE_LIKE_COUNT = 12;
    
private:
    TestSession _currentSession;
    bool _sessionActive;
    
    // Helper functions
    static float xyzToLab(float value, bool isY = false);
    static void rgbToXyz(uint8_t r, uint8_t g, uint8_t b, float xyz[3]);
    static void xyzToLabArray(const float xyz[3], float lab[3]);
    static float calculateAccuracy(float deltaE, float tolerance);
};

// Predefined swatch definitions
const SwatchTesting::ColorSwatch SwatchTesting::BASIC_RGB_SWATCHES[] = {
    {"Pure Red",     255,   0,   0, 0.412f, 0.213f, 0.019f, 5.0f, true},
    {"Pure Green",     0, 255,   0, 0.358f, 0.715f, 0.119f, 5.0f, true},
    {"Pure Blue",      0,   0, 255, 0.180f, 0.072f, 0.950f, 5.0f, true},
    {"White",        255, 255, 255, 0.950f, 1.000f, 1.089f, 3.0f, true},
    {"Black",          0,   0,   0, 0.000f, 0.000f, 0.000f, 2.0f, true},
    {"Gray 50%",     128, 128, 128, 0.203f, 0.214f, 0.233f, 4.0f, true},
    {"Yellow",       255, 255,   0, 0.770f, 0.928f, 0.138f, 4.0f, true},
    {"Cyan",           0, 255, 255, 0.538f, 0.787f, 1.069f, 4.0f, true}
};

const SwatchTesting::ColorSwatch SwatchTesting::EXTENDED_SWATCHES[] = {
    // Basic colors
    {"Pure Red",     255,   0,   0, 0.412f, 0.213f, 0.019f, 5.0f, true},
    {"Pure Green",     0, 255,   0, 0.358f, 0.715f, 0.119f, 5.0f, true},
    {"Pure Blue",      0,   0, 255, 0.180f, 0.072f, 0.950f, 5.0f, true},
    {"White",        255, 255, 255, 0.950f, 1.000f, 1.089f, 3.0f, true},
    {"Black",          0,   0,   0, 0.000f, 0.000f, 0.000f, 2.0f, true},
    
    // Secondary colors
    {"Yellow",       255, 255,   0, 0.770f, 0.928f, 0.138f, 4.0f, true},
    {"Cyan",           0, 255, 255, 0.538f, 0.787f, 1.069f, 4.0f, true},
    {"Magenta",      255,   0, 255, 0.592f, 0.285f, 0.969f, 4.0f, true},
    
    // Grays
    {"Gray 25%",      64,  64,  64, 0.051f, 0.054f, 0.058f, 4.0f, true},
    {"Gray 50%",     128, 128, 128, 0.203f, 0.214f, 0.233f, 4.0f, true},
    {"Gray 75%",     192, 192, 192, 0.457f, 0.481f, 0.524f, 4.0f, true},
    
    // Common colors
    {"Orange",       255, 165,   0, 0.000f, 0.000f, 0.000f, 6.0f, false},
    {"Purple",       128,   0, 128, 0.000f, 0.000f, 0.000f, 6.0f, false},
    {"Brown",        165,  42,  42, 0.000f, 0.000f, 0.000f, 6.0f, false},
    {"Pink",         255, 192, 203, 0.000f, 0.000f, 0.000f, 6.0f, false},
    {"Navy",           0,   0, 128, 0.000f, 0.000f, 0.000f, 6.0f, false}
};

const SwatchTesting::ColorSwatch SwatchTesting::PANTONE_LIKE_SWATCHES[] = {
    {"Pantone Red",    237,  41,  57, 0.000f, 0.000f, 0.000f, 5.0f, false},
    {"Pantone Blue",    0,  114, 188, 0.000f, 0.000f, 0.000f, 5.0f, false},
    {"Pantone Green",  0,  158,  96, 0.000f, 0.000f, 0.000f, 5.0f, false},
    {"Pantone Orange", 255, 103,  31, 0.000f, 0.000f, 0.000f, 5.0f, false},
    {"Pantone Purple", 102,  45, 145, 0.000f, 0.000f, 0.000f, 5.0f, false},
    {"Pantone Yellow", 254, 221,   0, 0.000f, 0.000f, 0.000f, 5.0f, false},
    {"Pantone Pink",   246, 138, 171, 0.000f, 0.000f, 0.000f, 5.0f, false},
    {"Pantone Teal",    0, 131, 143, 0.000f, 0.000f, 0.000f, 5.0f, false},
    {"Pantone Brown",  121,  85,  72, 0.000f, 0.000f, 0.000f, 6.0f, false},
    {"Pantone Gray",   147, 149, 152, 0.000f, 0.000f, 0.000f, 4.0f, false},
    {"Pantone Lime",   187, 219,  86, 0.000f, 0.000f, 0.000f, 5.0f, false},
    {"Pantone Coral",  255, 111,  97, 0.000f, 0.000f, 0.000f, 5.0f, false}
};

#endif // SWATCH_TESTING_H
