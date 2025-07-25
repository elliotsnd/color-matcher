/**
 * @file CIEDE2000.cpp
 * @brief Implementation of CIEDE2000 color difference calculation engine
 */

#include "CIEDE2000.h"

// Constructor
CIEDE2000::CIEDE2000() {
    // Initialize with default quality thresholds
    thresholds.excellent = 1.0f;
    thresholds.good = 2.0f;
    thresholds.acceptable = 3.0f;
    thresholds.poor = 5.0f;
}

// Apply gamma correction for sRGB
float CIEDE2000::applySRGBGamma(float value) const {
    if (value <= 0.04045f) {
        return value / 12.92f;
    } else {
        return pow((value + 0.055f) / 1.055f, 2.4f);
    }
}

// Remove gamma correction for sRGB
float CIEDE2000::removeSRGBGamma(float value) const {
    if (value <= 0.0031308f) {
        return value * 12.92f;
    } else {
        return 1.055f * pow(value, 1.0f / 2.4f) - 0.055f;
    }
}

// XYZ to LAB conversion helper
float CIEDE2000::xyzToLabHelper(float t) const {
    const float delta = 6.0f / 29.0f;
    const float deltaSquared = delta * delta;
    const float deltaCubed = deltaSquared * delta;
    
    if (t > deltaCubed) {
        return pow(t, 1.0f / 3.0f);
    } else {
        return (t / (3.0f * deltaSquared)) + (4.0f / 29.0f);
    }
}

// LAB to XYZ conversion helper
float CIEDE2000::labToXyzHelper(float t) const {
    const float delta = 6.0f / 29.0f;
    
    if (t > delta) {
        return t * t * t;
    } else {
        return 3.0f * delta * delta * (t - 4.0f / 29.0f);
    }
}

// Calculate hue angle with proper quadrant handling
float CIEDE2000::calculateHueAngle(float a, float b) const {
    if (a == 0 && b == 0) {
        return 0; // Undefined hue for achromatic colors
    }
    
    float hue = atan2(b, a) * 180.0f / PI;
    if (hue < 0) {
        hue += 360.0f;
    }
    return hue;
}

// Calculate hue difference with proper handling of circular nature
float CIEDE2000::calculateHueDifference(float h1, float h2) const {
    float deltaH = h2 - h1;
    
    if (abs(deltaH) > 180.0f) {
        if (deltaH > 180.0f) {
            deltaH -= 360.0f;
        } else {
            deltaH += 360.0f;
        }
    }
    
    return deltaH;
}

// Convert RGB to XYZ color space
XYZColor CIEDE2000::rgbToXYZ(const RGBColor& rgb, const String& colorSpace) const {
    // Normalize RGB values to 0-1 range
    float r = rgb.R / 255.0f;
    float g = rgb.G / 255.0f;
    float b = rgb.B / 255.0f;
    
    // Apply gamma correction (assuming sRGB for now)
    r = applySRGBGamma(r);
    g = applySRGBGamma(g);
    b = applySRGBGamma(b);
    
    // Convert to XYZ using sRGB matrix
    float x = sRGB_to_XYZ[0][0] * r + sRGB_to_XYZ[0][1] * g + sRGB_to_XYZ[0][2] * b;
    float y = sRGB_to_XYZ[1][0] * r + sRGB_to_XYZ[1][1] * g + sRGB_to_XYZ[1][2] * b;
    float z = sRGB_to_XYZ[2][0] * r + sRGB_to_XYZ[2][1] * g + sRGB_to_XYZ[2][2] * b;
    
    // Scale to 0-100 range
    return XYZColor(x * 100.0f, y * 100.0f, z * 100.0f);
}

// Convert XYZ to LAB color space
LABColor CIEDE2000::xyzToLAB(const XYZColor& xyz, const String& illuminant) const {
    // Normalize by illuminant (using D65)
    float xn = xyz.X / D65_X;
    float yn = xyz.Y / D65_Y;
    float zn = xyz.Z / D65_Z;
    
    // Apply LAB transformation
    float fx = xyzToLabHelper(xn);
    float fy = xyzToLabHelper(yn);
    float fz = xyzToLabHelper(zn);
    
    // Calculate LAB values
    float L = 116.0f * fy - 16.0f;
    float a = 500.0f * (fx - fy);
    float b = 200.0f * (fy - fz);
    
    return LABColor(L, a, b);
}

// Convert LAB to LCH color space
LCHColor CIEDE2000::labToLCH(const LABColor& lab) const {
    float C = sqrt(lab.a * lab.a + lab.b * lab.b);
    float H = calculateHueAngle(lab.a, lab.b);
    
    return LCHColor(lab.L, C, H);
}

// Convert RGB directly to LAB
LABColor CIEDE2000::rgbToLAB(const RGBColor& rgb, const String& colorSpace) const {
    XYZColor xyz = rgbToXYZ(rgb, colorSpace);
    return xyzToLAB(xyz);
}

// CIEDE2000 calculation helper functions
float CIEDE2000::calculateSL(float L) const {
    return 1.0f + (0.015f * pow(L - 50.0f, 2.0f)) / sqrt(20.0f + pow(L - 50.0f, 2.0f));
}

float CIEDE2000::calculateSC(float C) const {
    return 1.0f + 0.045f * C;
}

float CIEDE2000::calculateSH(float C, float H) const {
    float T = 1.0f - 0.17f * cos((H - 30.0f) * PI / 180.0f) +
              0.24f * cos(2.0f * H * PI / 180.0f) +
              0.32f * cos((3.0f * H + 6.0f) * PI / 180.0f) -
              0.20f * cos((4.0f * H - 63.0f) * PI / 180.0f);
    
    return 1.0f + 0.015f * C * T;
}

float CIEDE2000::calculateRT(float deltaTheta) const {
    float RC = 2.0f * sqrt(pow(25.0f, 7.0f) / (pow(25.0f, 7.0f) + pow(25.0f, 7.0f)));
    return -RC * sin(2.0f * deltaTheta);
}

// Calculate CIEDE2000 color difference
ColorDifferenceResult CIEDE2000::calculateDeltaE2000(const LABColor& lab1, const LABColor& lab2) const {
    ColorDifferenceResult result;
    
    // Convert to LCH
    LCHColor lch1 = labToLCH(lab1);
    LCHColor lch2 = labToLCH(lab2);
    
    // Calculate differences
    float deltaL = lch2.L - lch1.L;
    float deltaC = lch2.C - lch1.C;
    float deltaH = calculateHueDifference(lch1.H, lch2.H);
    
    // Calculate average values
    float avgL = (lch1.L + lch2.L) / 2.0f;
    float avgC = (lch1.C + lch2.C) / 2.0f;
    float avgH = (lch1.H + lch2.H) / 2.0f;
    
    // Handle hue average for circular nature
    if (abs(lch1.H - lch2.H) > 180.0f) {
        if (avgH < 180.0f) {
            avgH += 180.0f;
        } else {
            avgH -= 180.0f;
        }
    }
    
    // Calculate weighting functions
    float SL = calculateSL(avgL);
    float SC = calculateSC(avgC);
    float SH = calculateSH(avgC, avgH);
    
    // Calculate rotation term
    float deltaTheta = 30.0f * exp(-pow((avgH - 275.0f) / 25.0f, 2.0f));
    float RT = calculateRT(deltaTheta * PI / 180.0f);
    
    // Calculate CIEDE2000
    float kL = 1.0f, kC = 1.0f, kH = 1.0f; // Weighting factors (usually 1.0)
    
    float term1 = deltaL / (kL * SL);
    float term2 = deltaC / (kC * SC);
    float term3 = deltaH / (kH * SH);
    float term4 = RT * term2 * term3;
    
    result.deltaE2000 = sqrt(term1 * term1 + term2 * term2 + term3 * term3 + term4);
    result.deltaL = deltaL;
    result.deltaC = deltaC;
    result.deltaH = deltaH;
    
    // Calculate CIE76 for comparison
    result.deltaE76 = calculateDeltaE76(lab1, lab2);
    
    // Assess quality
    result.qualityLevel = assessColorQuality(result.deltaE2000);
    result.acceptable = (result.deltaE2000 <= thresholds.acceptable);
    
    return result;
}

// Calculate CIEDE2000 color difference from RGB colors
ColorDifferenceResult CIEDE2000::calculateDeltaE2000(const RGBColor& rgb1, const RGBColor& rgb2, 
                                                     const String& colorSpace) const {
    LABColor lab1 = rgbToLAB(rgb1, colorSpace);
    LABColor lab2 = rgbToLAB(rgb2, colorSpace);
    return calculateDeltaE2000(lab1, lab2);
}

// Calculate CIE76 ΔE*ab for comparison
float CIEDE2000::calculateDeltaE76(const LABColor& lab1, const LABColor& lab2) const {
    float deltaL = lab2.L - lab1.L;
    float deltaA = lab2.a - lab1.a;
    float deltaB = lab2.b - lab1.b;
    
    return sqrt(deltaL * deltaL + deltaA * deltaA + deltaB * deltaB);
}

// Assess color quality based on CIEDE2000 difference
String CIEDE2000::assessColorQuality(float deltaE) const {
    if (deltaE <= thresholds.excellent) {
        return "Excellent";
    } else if (deltaE <= thresholds.good) {
        return "Good";
    } else if (deltaE <= thresholds.acceptable) {
        return "Acceptable";
    } else if (deltaE <= thresholds.poor) {
        return "Poor";
    } else {
        return "Unacceptable";
    }
}

// Check if color difference is acceptable for given application
bool CIEDE2000::isAcceptableForApplication(float deltaE, const String& application) const {
    if (application == "critical" || application == "medical") {
        return deltaE <= 1.0f;
    } else if (application == "printing" || application == "photography") {
        return deltaE <= 2.0f;
    } else if (application == "display" || application == "general") {
        return deltaE <= 3.0f;
    } else if (application == "industrial") {
        return deltaE <= 5.0f;
    }
    
    return deltaE <= thresholds.acceptable;
}

// Calculate batch color differences
float CIEDE2000::calculateBatchDifferences(const LABColor* referenceColors, const LABColor* measuredColors,
                                          int count, ColorDifferenceResult* results) const {
    float totalDeltaE = 0.0f;
    
    for (int i = 0; i < count; i++) {
        results[i] = calculateDeltaE2000(referenceColors[i], measuredColors[i]);
        totalDeltaE += results[i].deltaE2000;
    }
    
    return totalDeltaE / count;
}

// Update quality thresholds
void CIEDE2000::updateQualityThresholds(float excellent, float good, float acceptable, float poor) {
    thresholds.excellent = excellent;
    thresholds.good = good;
    thresholds.acceptable = acceptable;
    thresholds.poor = poor;
}

// Get current quality thresholds
void CIEDE2000::getQualityThresholds(float& excellent, float& good, float& acceptable, float& poor) const {
    excellent = thresholds.excellent;
    good = thresholds.good;
    acceptable = thresholds.acceptable;
    poor = thresholds.poor;
}

// Validate LAB color values
bool CIEDE2000::validateLABColor(const LABColor& lab) const {
    return (lab.L >= 0 && lab.L <= 100 && 
            lab.a >= -128 && lab.a <= 127 && 
            lab.b >= -128 && lab.b <= 127);
}

// Get detailed color difference analysis
String CIEDE2000::getDetailedAnalysis(const ColorDifferenceResult& result) const {
    String analysis = "Color Difference Analysis:\n";
    analysis += "CIEDE2000 ΔE: " + String(result.deltaE2000, 2) + "\n";
    analysis += "CIE76 ΔE*ab: " + String(result.deltaE76, 2) + "\n";
    analysis += "Quality Level: " + result.qualityLevel + "\n";
    analysis += "Acceptable: " + String(result.acceptable ? "Yes" : "No") + "\n";
    analysis += "Component Differences:\n";
    analysis += "  ΔL*: " + String(result.deltaL, 2) + " (Lightness)\n";
    analysis += "  ΔC*: " + String(result.deltaC, 2) + " (Chroma)\n";
    analysis += "  ΔH*: " + String(result.deltaH, 2) + " (Hue)\n";
    
    return analysis;
}
