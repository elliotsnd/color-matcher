/**
 * @file SwatchTesting.cpp
 * @brief Implementation of RGB color swatch testing framework
 */

#include "SwatchTesting.h"
#include <math.h>

SwatchTesting::SwatchTesting() : _sessionActive(false) {
    // Initialize session
    _currentSession.numSwatches = 0;
    _currentSession.averageDeltaE = 0.0f;
    _currentSession.averageAccuracy = 0.0f;
    _currentSession.passedCount = 0;
    _currentSession.failedCount = 0;
    _currentSession.sessionPassed = false;
}

bool SwatchTesting::beginTestSession() {
    _sessionActive = true;
    _currentSession.numSwatches = 0;
    _currentSession.passedCount = 0;
    _currentSession.failedCount = 0;
    _currentSession.averageDeltaE = 0.0f;
    _currentSession.averageAccuracy = 0.0f;
    _currentSession.worstDeltaE = 0.0f;
    _currentSession.bestDeltaE = 999.0f;
    _currentSession.sessionPassed = false;
    
    return true;
}

SwatchTesting::SwatchResult SwatchTesting::testSwatch(const ColorSwatch& swatch, 
                                                     const ColorScience::RGBColor& measuredRGB) {
    SwatchResult result;
    result.swatch = swatch;
    result.measuredR = measuredRGB.r8;
    result.measuredG = measuredRGB.g8;
    result.measuredB = measuredRGB.b8;
    
    // Calculate Delta E (color difference)
    float referenceLab[3], measuredLab[3];
    rgbToLab(swatch.referenceR, swatch.referenceG, swatch.referenceB, referenceLab);
    rgbToLab(measuredRGB.r8, measuredRGB.g8, measuredRGB.b8, measuredLab);
    result.deltaE = calculateDeltaE(referenceLab, measuredLab);
    
    // Calculate RGB distance
    result.rgbError = calculateRGBDistance(swatch.referenceR, swatch.referenceG, swatch.referenceB,
                                          measuredRGB.r8, measuredRGB.g8, measuredRGB.b8);
    
    // Determine pass/fail based on tolerance
    result.passed = (result.deltaE <= swatch.tolerance);
    
    // Calculate accuracy percentage
    result.accuracy = calculateAccuracy(result.deltaE, swatch.tolerance);
    
    // Add to current session if active
    if (_sessionActive && _currentSession.numSwatches < 32) {
        _currentSession.results[_currentSession.numSwatches] = result;
        _currentSession.numSwatches++;
        
        if (result.passed) {
            _currentSession.passedCount++;
        } else {
            _currentSession.failedCount++;
        }
    }
    
    return result;
}

SwatchTesting::TestSession SwatchTesting::finalizeTestSession() {
    if (!_sessionActive) {
        return _currentSession;
    }
    
    // Calculate session statistics
    float totalDeltaE = 0.0f;
    float totalAccuracy = 0.0f;
    _currentSession.worstDeltaE = 0.0f;
    _currentSession.bestDeltaE = 999.0f;
    
    for (uint8_t i = 0; i < _currentSession.numSwatches; i++) {
        SwatchResult& result = _currentSession.results[i];
        totalDeltaE += result.deltaE;
        totalAccuracy += result.accuracy;
        
        if (result.deltaE > _currentSession.worstDeltaE) {
            _currentSession.worstDeltaE = result.deltaE;
        }
        if (result.deltaE < _currentSession.bestDeltaE) {
            _currentSession.bestDeltaE = result.deltaE;
        }
    }
    
    if (_currentSession.numSwatches > 0) {
        _currentSession.averageDeltaE = totalDeltaE / _currentSession.numSwatches;
        _currentSession.averageAccuracy = totalAccuracy / _currentSession.numSwatches;
        
        // Session passes if >80% of swatches pass and average Delta E < 6.0
        float passRate = (float)_currentSession.passedCount / _currentSession.numSwatches;
        _currentSession.sessionPassed = (passRate >= 0.8f) && (_currentSession.averageDeltaE < 6.0f);
    }
    
    _sessionActive = false;
    return _currentSession;
}

float SwatchTesting::calculateDeltaE(const float lab1[3], const float lab2[3]) {
    // CIE76 Delta E formula
    float deltaL = lab1[0] - lab2[0];
    float deltaA = lab1[1] - lab2[1];
    float deltaB = lab1[2] - lab2[2];
    
    return sqrt(deltaL * deltaL + deltaA * deltaA + deltaB * deltaB);
}

void SwatchTesting::rgbToLab(uint8_t red, uint8_t green, uint8_t blue, float lab[3]) {
    // Convert RGB to XYZ first
    float xyz[3];
    rgbToXyz(red, green, blue, xyz);
    
    // Convert XYZ to LAB
    xyzToLabArray(xyz, lab);
}

float SwatchTesting::calculateRGBDistance(uint8_t r1, uint8_t g1, uint8_t b1, 
                                         uint8_t r2, uint8_t g2, uint8_t b2) {
    float dr = (float)(r1 - r2);
    float dg = (float)(g1 - g2);
    float db = (float)(b1 - b2);
    
    return sqrt(dr * dr + dg * dg + db * db);
}

void SwatchTesting::printSwatchResult(const SwatchResult& result) {
    Serial.println("=== SWATCH TEST RESULT ===");
    Serial.printf("Swatch: %s\n", result.swatch.name);
    Serial.printf("Reference RGB: (%3d, %3d, %3d)\n", 
                 result.swatch.referenceR, result.swatch.referenceG, result.swatch.referenceB);
    Serial.printf("Measured RGB:  (%3d, %3d, %3d)\n", 
                 result.measuredR, result.measuredG, result.measuredB);
    Serial.printf("Delta E: %.2f (tolerance: %.1f)\n", result.deltaE, result.swatch.tolerance);
    Serial.printf("RGB Error: %.1f\n", result.rgbError);
    Serial.printf("Accuracy: %.1f%%\n", result.accuracy);
    Serial.printf("Result: %s\n", result.passed ? "PASS ✓" : "FAIL ✗");
    Serial.println();
}

void SwatchTesting::printTestSession(const TestSession& session) {
    Serial.println("=== TEST SESSION RESULTS ===");
    Serial.printf("Total Swatches: %d\n", session.numSwatches);
    Serial.printf("Passed: %d, Failed: %d\n", session.passedCount, session.failedCount);
    Serial.printf("Pass Rate: %.1f%%\n", 
                 session.numSwatches > 0 ? (float)session.passedCount / session.numSwatches * 100.0f : 0.0f);
    Serial.printf("Average Delta E: %.2f\n", session.averageDeltaE);
    Serial.printf("Average Accuracy: %.1f%%\n", session.averageAccuracy);
    Serial.printf("Best Delta E: %.2f\n", session.bestDeltaE);
    Serial.printf("Worst Delta E: %.2f\n", session.worstDeltaE);
    Serial.printf("Overall Result: %s\n", session.sessionPassed ? "PASS ✓" : "FAIL ✗");
    Serial.println();
    
    // Print individual results
    for (uint8_t i = 0; i < session.numSwatches; i++) {
        Serial.printf("%2d. %-15s: ΔE=%.1f %s\n", 
                     i + 1, 
                     session.results[i].swatch.name,
                     session.results[i].deltaE,
                     session.results[i].passed ? "✓" : "✗");
    }
    Serial.println();
}

const SwatchTesting::ColorSwatch* SwatchTesting::getStandardSwatches(uint8_t swatchSet, uint8_t& count) {
    switch (swatchSet) {
        case 0:
            count = BASIC_RGB_COUNT;
            return BASIC_RGB_SWATCHES;
        case 1:
            count = EXTENDED_COUNT;
            return EXTENDED_SWATCHES;
        case 2:
            count = PANTONE_LIKE_COUNT;
            return PANTONE_LIKE_SWATCHES;
        default:
            count = 0;
            return nullptr;
    }
}

// Private helper functions
float SwatchTesting::xyzToLab(float value, bool isY) {
    const float threshold = 0.008856f;
    const float factor = 7.787f;
    const float offset = 16.0f / 116.0f;
    
    if (value > threshold) {
        return pow(value, 1.0f / 3.0f);
    } else {
        return factor * value + offset;
    }
}

void SwatchTesting::rgbToXyz(uint8_t r, uint8_t g, uint8_t b, float xyz[3]) {
    // Normalize RGB to 0-1
    float rNorm = r / 255.0f;
    float gNorm = g / 255.0f;
    float bNorm = b / 255.0f;
    
    // Apply gamma correction (inverse sRGB)
    rNorm = (rNorm > 0.04045f) ? pow((rNorm + 0.055f) / 1.055f, 2.4f) : rNorm / 12.92f;
    gNorm = (gNorm > 0.04045f) ? pow((gNorm + 0.055f) / 1.055f, 2.4f) : gNorm / 12.92f;
    bNorm = (bNorm > 0.04045f) ? pow((bNorm + 0.055f) / 1.055f, 2.4f) : bNorm / 12.92f;
    
    // Convert to XYZ using sRGB matrix
    xyz[0] = 0.4124f * rNorm + 0.3576f * gNorm + 0.1805f * bNorm;
    xyz[1] = 0.2126f * rNorm + 0.7152f * gNorm + 0.0722f * bNorm;
    xyz[2] = 0.0193f * rNorm + 0.1192f * gNorm + 0.9505f * bNorm;
}

void SwatchTesting::xyzToLabArray(const float xyz[3], float lab[3]) {
    // D65 white point
    const float Xn = 0.95047f;
    const float Yn = 1.00000f;
    const float Zn = 1.08883f;
    
    float fx = xyzToLab(xyz[0] / Xn);
    float fy = xyzToLab(xyz[1] / Yn);
    float fz = xyzToLab(xyz[2] / Zn);
    
    lab[0] = 116.0f * fy - 16.0f;  // L
    lab[1] = 500.0f * (fx - fy);   // a
    lab[2] = 200.0f * (fy - fz);   // b
}

float SwatchTesting::calculateAccuracy(float deltaE, float tolerance) {
    if (deltaE <= tolerance) {
        return 100.0f - (deltaE / tolerance) * 20.0f;  // 80-100% for passing values
    } else {
        float excess = deltaE - tolerance;
        float penalty = (excess / tolerance) * 40.0f;  // Penalty for exceeding tolerance
        return max(0.0f, 80.0f - penalty);
    }
}
