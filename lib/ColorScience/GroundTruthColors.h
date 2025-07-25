/*!
 * @file GroundTruthColors.h
 * @brief Professional Ground Truth sRGB Values for Calibration References
 * @copyright Professional Color Calibration System
 * @version 2.0
 * @date 2024
 * 
 * This file defines the true, standard sRGB values of professional reference materials.
 * These values MUST be known and accurate for proper Color Correction Matrix generation.
 */

#pragma once

#include "Arduino.h"

namespace ColorCalibration {

/**
 * @brief RGB Color structure for ground truth values
 */
struct RGBColor {
    uint8_t r, g, b;
    
    RGBColor(uint8_t red = 0, uint8_t green = 0, uint8_t blue = 0) 
        : r(red), g(green), b(blue) {}
};

/**
 * @brief Professional Ground Truth Colors
 * 
 * These are the actual sRGB values of your professional reference materials.
 * Replace these values with the certified sRGB values from your color standards.
 * 
 * For professional color measurement, these should be:
 * - Measured with a calibrated spectrophotometer
 * - Under D65 illuminant conditions
 * - Converted to sRGB color space
 * - Verified against color standards (X-Rite, Pantone, etc.)
 */
struct GroundTruthColors {
    // Professional reference values - REPLACE WITH YOUR ACTUAL CERTIFIED VALUES
    
    /**
     * @brief Black Reference - True black definition
     * When sensor reads black calibration values, return this RGB
     */
    const RGBColor black = RGBColor(0, 0, 0);

    /**
     * @brief White Reference - True white definition
     * When sensor reads white calibration values, return this RGB
     */
    const RGBColor white = RGBColor(255, 255, 255);

    /**
     * @brief Blue Reference - True blue definition
     * When sensor reads blue calibration values, return this RGB
     */
    const RGBColor blue = RGBColor(0, 0, 255);

    /**
     * @brief Yellow Reference - True yellow definition
     * When sensor reads yellow calibration values, return this RGB
     */
    const RGBColor yellow = RGBColor(255, 255, 0);
    
    /**
     * @brief Validation method to check if ground truth values are reasonable
     * @return true if values appear to be professional standards
     */
    bool validateGroundTruth() const {
        // Basic sanity checks for professional color standards
        bool blackValid = (black.r <= 10 && black.g <= 10 && black.b <= 10);
        bool whiteValid = (white.r >= 240 && white.g >= 240 && white.b >= 240);
        bool blueValid = (blue.b > blue.r && blue.b > blue.g);
        bool yellowValid = (yellow.r >= 200 && yellow.g >= 180 && yellow.b <= 50);
        
        return blackValid && whiteValid && blueValid && yellowValid;
    }
    
    /**
     * @brief Get ground truth color by reference type
     * @param referenceType Type of reference (0=black, 1=white, 2=blue, 3=yellow)
     * @return RGBColor for the specified reference
     */
    RGBColor getReference(int referenceType) const {
        switch (referenceType) {
            case 0: return black;
            case 1: return white;
            case 2: return blue;
            case 3: return yellow;
            default: return RGBColor(0, 0, 0);
        }
    }
};

// Global instance of ground truth colors
extern const GroundTruthColors groundTruth;

} // namespace ColorCalibration
