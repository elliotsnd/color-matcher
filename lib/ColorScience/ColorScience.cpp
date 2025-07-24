/**
 * @file ColorScience.cpp
 * @brief Implementation of advanced color science library
 */

#include "ColorScience.h"

ColorScience::ColorScience() {
    // Constructor - nothing to initialize for static class
}

ColorScience::RGBColor ColorScience::xyzToRGB(const XYZColor& xyz, const IRData& irData, 
                                             const CalibrationData& calibData) {
    RGBColor result;
    
    // Step 1: Apply ambient compensation (black subtraction)
    XYZColor compensatedXYZ = applyAmbientCompensation(xyz, calibData);
    
    // Step 2: Apply adaptive IR compensation
    compensatedXYZ = applyIRCompensation(compensatedXYZ, irData, calibData);
    
    // Step 3: Normalize using white point
    compensatedXYZ = normalizeXYZ(compensatedXYZ, calibData.whiteReference);
    
    // Step 4: Select conversion matrix
    const float* matrix = calibData.useCustomMatrix ? calibData.customMatrix :
                         (calibData.useAdobeRGB ? XYZ_TO_ADOBE_RGB_MATRIX : XYZ_TO_S_RGB_MATRIX);
    
    // Step 5: Apply matrix transformation
    float xyzArray[3] = {compensatedXYZ.X, compensatedXYZ.Y, compensatedXYZ.Z};
    float rgbLinear[3];
    matrixMultiply3x3(matrix, xyzArray, rgbLinear);
    
    // Step 6: Clamp to valid range
    rgbLinear[0] = clamp(rgbLinear[0]);
    rgbLinear[1] = clamp(rgbLinear[1]);
    rgbLinear[2] = clamp(rgbLinear[2]);
    
    // Step 7: Apply gamma correction (linear to sRGB)
    result.r = applySRGBGamma(rgbLinear[0]);
    result.g = applySRGBGamma(rgbLinear[1]);
    result.b = applySRGBGamma(rgbLinear[2]);
    
    // Step 8: Convert to 8-bit values
    floatToRGB8(result.r, result.g, result.b, result.r8, result.g8, result.b8);
    
    return result;
}

ColorScience::XYZColor ColorScience::rawToXYZ(uint16_t rawX, uint16_t rawY, uint16_t rawZ,
                                             uint16_t rawIR1, uint16_t rawIR2,
                                             const CalibrationData& calibData) {
    XYZColor result;
    
    // Convert raw values to normalized 0-1 range
    // Using 16-bit max as reference (could be adjusted based on integration time)
    result.X = static_cast<float>(rawX) / 65535.0f;
    result.Y = static_cast<float>(rawY) / 65535.0f;
    result.Z = static_cast<float>(rawZ) / 65535.0f;
    
    return result;
}

float ColorScience::applySRGBGamma(float linear) {
    if (linear <= GAMMA_THRESHOLD) {
        return linear * GAMMA_LINEAR_COEFF;
    } else {
        return GAMMA_SCALE * pow(linear, GAMMA_POWER) - GAMMA_OFFSET_A;
    }
}

float ColorScience::applyInverseSRGBGamma(float gamma) {
    if (gamma <= GAMMA_THRESHOLD * GAMMA_LINEAR_COEFF) {
        return gamma / GAMMA_LINEAR_COEFF;
    } else {
        return pow((gamma + GAMMA_OFFSET_A) / GAMMA_SCALE, 1.0f / GAMMA_POWER);
    }
}

ColorScience::XYZColor ColorScience::normalizeXYZ(const XYZColor& xyz, const XYZColor& whitePoint) {
    XYZColor result;
    
    // Normalize by white point (Bradford adaptation could be added here)
    result.X = xyz.X / whitePoint.X;
    result.Y = xyz.Y / whitePoint.Y;
    result.Z = xyz.Z / whitePoint.Z;
    
    return result;
}

ColorScience::XYZColor ColorScience::applyIRCompensation(const XYZColor& xyz, const IRData& irData,
                                                        const CalibrationData& calibData) {
    XYZColor result = xyz;

    // LED-specific IR compensation
    if (calibData.ledIR.baseIRCompensation > 0.0f) {
        // Calculate LED brightness level from Y channel (luminance)
        float ledBrightness = xyz.Y;

        // Calculate dynamic IR compensation based on LED brightness
        float dynamicCompensation = calibData.ledIR.baseIRCompensation;

        if (calibData.ledIR.adaptToLEDBrightness) {
            // Adjust compensation based on LED brightness
            // Brighter LEDs typically have more IR leakage
            float brightnessEffect = ledBrightness * calibData.ledIR.ledBrightnessResponse;
            dynamicCompensation += brightnessEffect;

            // Clamp to min/max limits
            dynamicCompensation = clamp(dynamicCompensation,
                                      calibData.ledIR.minCompensation,
                                      calibData.ledIR.maxCompensation);
        }

        // Calculate IR contamination level
        float totalIR = (irData.IR1 + irData.IR2) / 2.0f;
        float irContamination = totalIR * dynamicCompensation;

        if (calibData.spectral.useChannelSpecificIR) {
            // Apply channel-specific IR compensation
            result.X -= result.X * calibData.spectral.xChannelIRLeakage * irContamination;
            result.Y -= result.Y * calibData.spectral.yChannelIRLeakage * irContamination;
            result.Z -= result.Z * calibData.spectral.zChannelIRLeakage * irContamination;
        } else {
            // Apply uniform IR compensation (legacy mode)
            float uniformCompensation = irContamination * 0.1f; // 10% base compensation
            result.X *= (1.0f - uniformCompensation * 0.5f);  // X moderately affected
            result.Y *= (1.0f - uniformCompensation * 0.3f);  // Y least affected
            result.Z *= (1.0f - uniformCompensation * 1.0f);  // Z most affected by IR
        }

        // Ensure non-negative values
        result.X = max(0.0f, result.X);
        result.Y = max(0.0f, result.Y);
        result.Z = max(0.0f, result.Z);
    }

    return result;
}

ColorScience::XYZColor ColorScience::applyAmbientCompensation(const XYZColor& xyz,
                                                             const CalibrationData& calibData) {
    XYZColor result;

    if (calibData.ambientCompensationEnabled) {
        // Subtract black reference (dark current and ambient light)
        result.X = xyz.X - calibData.blackReference.X;
        result.Y = xyz.Y - calibData.blackReference.Y;
        result.Z = xyz.Z - calibData.blackReference.Z;

        // Advanced edge case handling

        // 1. Ensure non-negative values with small tolerance
        const float minValue = 0.001f; // Prevent complete zero values
        result.X = max(minValue, result.X);
        result.Y = max(minValue, result.Y);
        result.Z = max(minValue, result.Z);

        // 2. Check for over-subtraction (indicates bad black reference)
        float overSubtractionThreshold = 0.1f; // 10% of original value
        if (result.X < xyz.X * overSubtractionThreshold ||
            result.Y < xyz.Y * overSubtractionThreshold ||
            result.Z < xyz.Z * overSubtractionThreshold) {

            // Reduce black reference impact if over-subtraction detected
            float reductionFactor = 0.5f; // Use 50% of black reference
            result.X = xyz.X - (calibData.blackReference.X * reductionFactor);
            result.Y = xyz.Y - (calibData.blackReference.Y * reductionFactor);
            result.Z = xyz.Z - (calibData.blackReference.Z * reductionFactor);

            // Re-apply minimum value constraint
            result.X = max(minValue, result.X);
            result.Y = max(minValue, result.Y);
            result.Z = max(minValue, result.Z);
        }

        // 3. Preserve color ratios if one channel becomes too small
        float minRatio = 0.01f; // Minimum 1% ratio between channels
        float maxChannel = max(max(result.X, result.Y), result.Z);

        if (result.X < maxChannel * minRatio) result.X = maxChannel * minRatio;
        if (result.Y < maxChannel * minRatio) result.Y = maxChannel * minRatio;
        if (result.Z < maxChannel * minRatio) result.Z = maxChannel * minRatio;

    } else {
        result = xyz;
    }

    return result;
}

void ColorScience::matrixMultiply3x3(const float matrix[9], const float input[3], float output[3]) {
    output[0] = matrix[0] * input[0] + matrix[1] * input[1] + matrix[2] * input[2];
    output[1] = matrix[3] * input[0] + matrix[4] * input[1] + matrix[5] * input[2];
    output[2] = matrix[6] * input[0] + matrix[7] * input[1] + matrix[8] * input[2];
}

float ColorScience::clamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

void ColorScience::floatToRGB8(float r, float g, float b, uint8_t& r8, uint8_t& g8, uint8_t& b8) {
    r8 = static_cast<uint8_t>(clamp(r * 255.0f, 0.0f, 255.0f));
    g8 = static_cast<uint8_t>(clamp(g * 255.0f, 0.0f, 255.0f));
    b8 = static_cast<uint8_t>(clamp(b * 255.0f, 0.0f, 255.0f));
}

float ColorScience::calculateColorTemperature(const XYZColor& xyz) {
    // Calculate chromaticity coordinates
    float x, y;
    calculateChromaticity(xyz, x, y);
    
    // McCamy's formula for color temperature
    float n = (x - 0.3320f) / (0.1858f - y);
    return 449.0f * n * n * n + 3525.0f * n * n + 6823.3f * n + 5520.33f;
}

void ColorScience::calculateChromaticity(const XYZColor& xyz, float& x, float& y) {
    float sum = xyz.X + xyz.Y + xyz.Z;
    if (sum > 0.0f) {
        x = xyz.X / sum;
        y = xyz.Y / sum;
    } else {
        x = 0.0f;
        y = 0.0f;
    }
}

bool ColorScience::validateCalibrationData(const CalibrationData& calibData) {
    // Check if white reference is valid (should be > black reference)
    if (calibData.whiteReference.Y <= calibData.blackReference.Y) {
        return false;
    }
    
    // Check if references are in reasonable range
    if (calibData.whiteReference.X < 0.0f || calibData.whiteReference.Y < 0.0f || 
        calibData.whiteReference.Z < 0.0f) {
        return false;
    }
    
    // Check IR compensation factor is reasonable
    if (calibData.irCompensationFactor < 0.0f || calibData.irCompensationFactor > 1.0f) {
        return false;
    }
    
    return true;
}

ColorScience::CalibrationData ColorScience::createDefaultCalibration() {
    CalibrationData calibData;

    // Default white reference (D65-like)
    calibData.whiteReference = {0.95047f, 1.00000f, 1.08883f};

    // Default black reference (minimal values)
    calibData.blackReference = {0.0f, 0.0f, 0.0f};

    // Default IR references
    calibData.whiteIR = {0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    calibData.blackIR = {0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};

    // LED-specific IR compensation defaults
    calibData.ledIR.baseIRCompensation = 0.08f;      // 8% base compensation for LED
    calibData.ledIR.ledBrightnessResponse = 0.02f;   // 2% additional per brightness unit
    calibData.ledIR.irToLedRatio = 0.15f;           // Expected 15% IR relative to LED
    calibData.ledIR.adaptToLEDBrightness = true;     // Enable adaptive compensation
    calibData.ledIR.minCompensation = 0.02f;         // Minimum 2% compensation
    calibData.ledIR.maxCompensation = 0.25f;         // Maximum 25% compensation

    // Spectral response defaults for LED (based on typical TCS3430 response)
    calibData.spectral.xChannelIRLeakage = 0.03f;    // 3% IR leakage in X channel
    calibData.spectral.yChannelIRLeakage = 0.015f;   // 1.5% IR leakage in Y channel
    calibData.spectral.zChannelIRLeakage = 0.08f;    // 8% IR leakage in Z channel (most affected)
    calibData.spectral.useChannelSpecificIR = true;  // Enable per-channel compensation

    // General settings
    calibData.ambientCompensationEnabled = true;
    calibData.useAdobeRGB = false;  // Use sRGB by default
    calibData.useCustomMatrix = false;

    return calibData;
}

// Private helper functions
float ColorScience::linearInterpolate(float x, float x1, float y1, float x2, float y2) {
    return y1 + (x - x1) * (y2 - y1) / (x2 - x1);
}

float ColorScience::calculateIRContamination(const IRData& irData) {
    // Advanced IR contamination model based on multiple factors

    // Factor 1: IR1/IR2 ratio (spectral signature)
    float ratio = (irData.IR2 > 0.0f) ? (irData.IR1 / irData.IR2) : 1.0f;
    float ratioContamination = clamp((ratio - 0.8f) / 1.5f, 0.0f, 1.0f);

    // Factor 2: Absolute IR levels (high IR = more contamination)
    float avgIR = (irData.IR1 + irData.IR2) / 2.0f;
    float levelContamination = clamp(avgIR * 2.0f, 0.0f, 1.0f);

    // Factor 3: IR1 vs IR2 difference (asymmetric IR sources)
    float irDifference = abs(irData.IR1 - irData.IR2);
    float asymmetryContamination = clamp(irDifference * 3.0f, 0.0f, 1.0f);

    // Weighted combination of factors
    float totalContamination = 0.5f * ratioContamination +
                              0.3f * levelContamination +
                              0.2f * asymmetryContamination;

    return clamp(totalContamination, 0.0f, 1.0f);
}

ColorScience::XYZColor ColorScience::applyWhiteBalance(const XYZColor& xyz, const XYZColor& whitePoint) {
    // Bradford chromatic adaptation could be implemented here
    // For now, simple scaling
    return normalizeXYZ(xyz, whitePoint);
}
