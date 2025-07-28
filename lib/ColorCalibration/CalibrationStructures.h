/**
 * @file CalibrationStructures.h
 * @brief Core data structures for 5-Point Color Correction Matrix (CCM) calibration
 * @author ESP32 Color Calibration System
 * @version 1.0.0
 * @date 2024
 * 
 * This file contains the fundamental data structures used throughout the
 * color calibration system, including calibration points, color correction
 * matrices, and target color definitions.
 */

#ifndef CALIBRATION_STRUCTURES_H
#define CALIBRATION_STRUCTURES_H

#include <Arduino.h>
#include <vector>

/**
 * @brief Structure representing a single calibration point
 * 
 * Contains raw sensor readings and target RGB values for color calibration
 */
struct CalibrationPoint {
    uint16_t rawX;      ///< Raw X sensor reading (0-65535)
    uint16_t rawY;      ///< Raw Y sensor reading (0-65535)
    uint16_t rawZ;      ///< Raw Z sensor reading (0-65535)
    uint8_t targetR;    ///< Target red value (0-255)
    uint8_t targetG;    ///< Target green value (0-255)
    uint8_t targetB;    ///< Target blue value (0-255)
    uint32_t timestamp; ///< Unix timestamp of calibration
    float quality;      ///< Quality score (0.0-1.0)
    
    /**
     * @brief Default constructor
     */
    CalibrationPoint() : rawX(0), rawY(0), rawZ(0), targetR(0), targetG(0), targetB(0), timestamp(0), quality(0.0f) {}
    
    /**
     * @brief Parameterized constructor
     */
    CalibrationPoint(uint16_t x, uint16_t y, uint16_t z, uint8_t r, uint8_t g, uint8_t b, uint32_t ts = 0, float q = 1.0f)
        : rawX(x), rawY(y), rawZ(z), targetR(r), targetG(g), targetB(b), timestamp(ts), quality(q) {}
};

/**
 * @brief Compensation levels for color correction matrix application
 *
 * Defines the different levels of compensation that can be applied
 * during color correction, from basic matrix-only to full professional pipeline.
 */
enum class CompensationLevel {
    NONE,           ///< Basic matrix transformation only (legacy compatibility)
    BLACK_ONLY,     ///< Matrix + black reference compensation + gamma correction
    PROFESSIONAL,   ///< Matrix + dark offset + flare compensation + gamma correction
    AUTO            ///< Automatically choose best available compensation level
};

/**
 * @brief 3x3 Color Correction Matrix structure
 *
 * Represents the transformation matrix for converting raw XYZ values
 * to corrected RGB values using least-squares approximation.
 *
 * UNIFIED INTERFACE: This structure now provides a single apply() method
 * that replaces the previous three redundant methods (apply, applyProfessional,
 * applyWithBlackCompensation) with configurable compensation levels.
 */
struct ColorCorrectionMatrix {
    float m[3][3];      ///< 3x3 transformation matrix
    bool isValid;       ///< Matrix validity flag
    float determinant;  ///< Matrix determinant for stability check
    float conditionNumber; ///< Condition number for quality assessment
    
    /**
     * @brief Default constructor - creates identity matrix
     */
    ColorCorrectionMatrix() : isValid(false), determinant(0.0f), conditionNumber(0.0f) {
        // Initialize as identity matrix
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                m[i][j] = (i == j) ? 1.0f : 0.0f;
            }
        }
    }
    
    /**
     * @brief UNIFIED color correction method with configurable compensation levels
     *
     * This single method replaces all previous apply methods (apply, applyProfessional,
     * applyWithBlackCompensation) with a unified interface that automatically selects
     * the appropriate compensation pipeline based on available calibration data.
     *
     * COMPENSATION LEVELS:
     * - NONE: Basic matrix transformation only (legacy compatibility)
     * - BLACK_ONLY: Matrix + black reference compensation + gamma correction
     * - PROFESSIONAL: Matrix + dark offset + flare compensation + gamma correction
     * - AUTO: Automatically choose best available level based on calibration data
     *
     * @param x Raw X value from sensor
     * @param y Raw Y value from sensor
     * @param z Raw Z value from sensor
     * @param r Output red value (0-255)
     * @param g Output green value (0-255)
     * @param b Output blue value (0-255)
     * @param level Compensation level to apply (default: AUTO)
     * @param darkOffset Optional dark current calibration point (for PROFESSIONAL level)
     * @param blackRef Optional black reference calibration point (for BLACK_ONLY and PROFESSIONAL)
     * @return true if transformation successful, false otherwise
     */
    bool apply(uint16_t x, uint16_t y, uint16_t z, uint8_t& r, uint8_t& g, uint8_t& b,
               CompensationLevel level = CompensationLevel::AUTO,
               const CalibrationPoint* darkOffset = nullptr,
               const CalibrationPoint* blackRef = nullptr) const {

        // === ENHANCED ERROR HANDLING AND VALIDATION ===

        // Guard clause: Check matrix validity
        if (!isValid) {
            Serial.println("⚠️ ColorCorrectionMatrix: Matrix invalid - using fallback conversion");
            // Fallback to simple normalization
            r = static_cast<uint8_t>(constrain(x / 256, 0, 255));
            g = static_cast<uint8_t>(constrain(y / 256, 0, 255));
            b = static_cast<uint8_t>(constrain(z / 256, 0, 255));
            return false;
        }

        // Guard clause: Validate input sensor readings
        if (x == 0 && y == 0 && z == 0) {
            Serial.println("⚠️ ColorCorrectionMatrix: Zero sensor input - outputting black");
            r = g = b = 0;
            return true; // This is a valid result for zero input
        }

        // Guard clause: Check for sensor overflow
        const uint16_t MAX_SAFE_VALUE = 65000;
        if (x > MAX_SAFE_VALUE || y > MAX_SAFE_VALUE || z > MAX_SAFE_VALUE) {
            Serial.println("⚠️ ColorCorrectionMatrix: Sensor overflow detected - clamping input values");
            // Note: We don't modify the input parameters, just warn
        }

        // Determine actual compensation level to use
        CompensationLevel actualLevel = level;
        if (level == CompensationLevel::AUTO) {
            // Automatically choose best available compensation level
            if (darkOffset != nullptr && blackRef != nullptr) {
                actualLevel = CompensationLevel::PROFESSIONAL;
            } else if (blackRef != nullptr) {
                actualLevel = CompensationLevel::BLACK_ONLY;
            } else {
                actualLevel = CompensationLevel::NONE;
            }
        }

        // Guard clause: Validate required calibration points for selected level
        if (actualLevel == CompensationLevel::PROFESSIONAL && (darkOffset == nullptr || blackRef == nullptr)) {
            Serial.println("⚠️ ColorCorrectionMatrix: PROFESSIONAL level requires both dark offset and black reference - falling back to BLACK_ONLY");
            actualLevel = (blackRef != nullptr) ? CompensationLevel::BLACK_ONLY : CompensationLevel::NONE;
        }

        if (actualLevel == CompensationLevel::BLACK_ONLY && blackRef == nullptr) {
            Serial.println("⚠️ ColorCorrectionMatrix: BLACK_ONLY level requires black reference - falling back to NONE");
            actualLevel = CompensationLevel::NONE;
        }

        // Apply the selected compensation pipeline with error handling
        try {
            switch (actualLevel) {
                case CompensationLevel::PROFESSIONAL:
                    // Silent operation - no spam logs
                    return applyProfessionalPipeline(x, y, z, *darkOffset, *blackRef, r, g, b);

                case CompensationLevel::BLACK_ONLY:
                    // Silent operation - no spam logs
                    return applyBlackCompensationPipeline(x, y, z, *blackRef, r, g, b);

                case CompensationLevel::NONE:
                default:
                    // Silent operation - no spam logs
                    return applyBasicPipeline(x, y, z, r, g, b);
            }
        } catch (...) {
            Serial.println("[CCM_PIPELINE] ❌ ColorCorrectionMatrix: Exception in compensation pipeline - using basic fallback");
            return applyBasicPipeline(x, y, z, r, g, b);
        }
    }

private:
    /**
     * @brief Apply basic matrix transformation pipeline (legacy compatibility)
     * @param x Raw X value from sensor
     * @param y Raw Y value from sensor
     * @param z Raw Z value from sensor
     * @param r Output red value (0-255)
     * @param g Output green value (0-255)
     * @param b Output blue value (0-255)
     * @return true if transformation successful
     */
    bool applyBasicPipeline(uint16_t x, uint16_t y, uint16_t z, uint8_t& r, uint8_t& g, uint8_t& b) const {
        // Normalize to 0-1 range (legacy behavior)
        float normalizedX = static_cast<float>(x) / 65535.0f;
        float normalizedY = static_cast<float>(y) / 65535.0f;
        float normalizedZ = static_cast<float>(z) / 65535.0f;

        // Apply matrix transformation: RGB = CCM * XYZ
        float rf = m[0][0] * normalizedX + m[0][1] * normalizedY + m[0][2] * normalizedZ;
        float gf = m[1][0] * normalizedX + m[1][1] * normalizedY + m[1][2] * normalizedZ;
        float bf = m[2][0] * normalizedX + m[2][1] * normalizedY + m[2][2] * normalizedZ;

        // Convert to 0-255 range
        float r255 = rf * 255.0f;
        float g255 = gf * 255.0f;
        float b255 = bf * 255.0f;

        // Smart scaling: If any channel exceeds 255, scale all proportionally
        float maxChannel = fmax(fmax(r255, g255), b255);
        if (maxChannel > 255.0f) {
            float scaleFactor = 255.0f / maxChannel;
            r255 *= scaleFactor;
            g255 *= scaleFactor;
            b255 *= scaleFactor;
            // Only log when scaling actually occurs (rare)
            Serial.println("[CCM_SCALING] 🎨 RGB scaling applied: factor=" + String(scaleFactor, 3) +
                          " (was " + String(maxChannel, 1) + ", now " + String(fmax(fmax(r255, g255), b255), 1) + ")");
        }

        // Apply final bounds checking (should not be needed after scaling)
        r = static_cast<uint8_t>(constrain(static_cast<int>(r255), 0, 255));
        g = static_cast<uint8_t>(constrain(static_cast<int>(g255), 0, 255));
        b = static_cast<uint8_t>(constrain(static_cast<int>(b255), 0, 255));

        return true;
    }

    /**
     * @brief Apply professional pipeline with dark current and flare compensation
     *
     * This method implements a professional-grade color correction pipeline:
     * Stage 1: Dark Current Subtraction - Remove inherent sensor noise
     * Stage 2: Flare Compensation - Remove light leakage from black reference
     * Stage 3: Matrix Transformation - Apply color correction matrix
     * Stage 4: Gamma Correction - Apply sRGB gamma curve
     *
     * @param x Raw X value from current reading
     * @param y Raw Y value from current reading
     * @param z Raw Z value from current reading
     * @param darkOffset Dark current calibration point (LED OFF)
     * @param blackRef Black reference calibration point (LED ON with black sample)
     * @param r Output red value (0-255)
     * @param g Output green value (0-255)
     * @param b Output blue value (0-255)
     * @return true if transformation successful, false otherwise
     */
    bool applyProfessionalPipeline(uint16_t x, uint16_t y, uint16_t z,
                                  const CalibrationPoint& darkOffset,
                                  const CalibrationPoint& blackRef,
                                  uint8_t& r, uint8_t& g, uint8_t& b) const {
        if (!isValid) return false;

        // --- Stage 1: Dark Current Subtraction ---
        // Remove the inherent sensor noise (dark current) measured with LED OFF
        float darkCompensatedX = fmax(0.0f, static_cast<float>(x) - static_cast<float>(darkOffset.rawX));
        float darkCompensatedY = fmax(0.0f, static_cast<float>(y) - static_cast<float>(darkOffset.rawY));
        float darkCompensatedZ = fmax(0.0f, static_cast<float>(z) - static_cast<float>(darkOffset.rawZ));

        // --- Stage 2: Calculate Flare Offset ---
        // The "flare" is the dark-compensated reading of the black reference (LED ON)
        float flareX = fmax(0.0f, static_cast<float>(blackRef.rawX) - static_cast<float>(darkOffset.rawX));
        float flareY = fmax(0.0f, static_cast<float>(blackRef.rawY) - static_cast<float>(darkOffset.rawY));
        float flareZ = fmax(0.0f, static_cast<float>(blackRef.rawZ) - static_cast<float>(darkOffset.rawZ));

        // --- Stage 3: Flare Subtraction ---
        // Remove the light leakage component from the current reading
        float flareCompensatedX = fmax(0.0f, darkCompensatedX - flareX);
        float flareCompensatedY = fmax(0.0f, darkCompensatedY - flareY);
        float flareCompensatedZ = fmax(0.0f, darkCompensatedZ - flareZ);

        // --- Stage 4: Normalize to 0-1 range ---
        // Find the maximum possible signal (white point minus flare)
        float maxSignal = 4095.0f; // Assuming 12-bit ADC, adjust if needed
        float normalizedX = flareCompensatedX / maxSignal;
        float normalizedY = flareCompensatedY / maxSignal;
        float normalizedZ = flareCompensatedZ / maxSignal;

        // --- Stage 5: Apply Color Correction Matrix ---
        // The matrix is now applied to the pure, flare-free color signal
        float linearR = m[0][0] * normalizedX + m[0][1] * normalizedY + m[0][2] * normalizedZ;
        float linearG = m[1][0] * normalizedX + m[1][1] * normalizedY + m[1][2] * normalizedZ;
        float linearB = m[2][0] * normalizedX + m[2][1] * normalizedY + m[2][2] * normalizedZ;

        // --- Stage 6: Apply sRGB Gamma Correction ---
        float gammaR = applySRGBGamma(fmax(0.0f, fmin(1.0f, linearR)));
        float gammaG = applySRGBGamma(fmax(0.0f, fmin(1.0f, linearG)));
        float gammaB = applySRGBGamma(fmax(0.0f, fmin(1.0f, linearB)));

        // --- Stage 7: Convert to 8-bit RGB with Smart Scaling ---
        float r255 = gammaR * 255.0f;
        float g255 = gammaG * 255.0f;
        float b255 = gammaB * 255.0f;

        // Smart scaling: If any channel exceeds 255, scale all proportionally
        float maxChannel = fmax(fmax(r255, g255), b255);
        if (maxChannel > 255.0f) {
            float scaleFactor = 255.0f / maxChannel;
            r255 *= scaleFactor;
            g255 *= scaleFactor;
            b255 *= scaleFactor;
            // Only log when scaling actually occurs (rare)
            Serial.println("[CCM_SCALING] 🎨 Professional RGB scaling applied: factor=" + String(scaleFactor, 3));
        }

        r = (uint8_t)constrain((int)r255, 0, 255);
        g = (uint8_t)constrain((int)g255, 0, 255);
        b = (uint8_t)constrain((int)b255, 0, 255);

        return true;
    }

    /**
     * @brief Apply black compensation pipeline with gamma correction
     * @param x Raw X value from current reading
     * @param y Raw Y value from current reading
     * @param z Raw Z value from current reading
     * @param blackRef Black reference calibration point for compensation
     * @param r Output red value (0-255)
     * @param g Output green value (0-255)
     * @param b Output blue value (0-255)
     * @return true if transformation successful, false otherwise
     */
    bool applyBlackCompensationPipeline(uint16_t x, uint16_t y, uint16_t z,
                                       const CalibrationPoint& blackRef,
                                       uint8_t& r, uint8_t& g, uint8_t& b) const {
        if (!isValid) return false;

        // --- Step 1: Black-Level Compensation ---
        // Subtract the black reference to remove sensor offset and flare
        float compensatedX = fmax(0.0f, static_cast<float>(x) - static_cast<float>(blackRef.rawX));
        float compensatedY = fmax(0.0f, static_cast<float>(y) - static_cast<float>(blackRef.rawY));
        float compensatedZ = fmax(0.0f, static_cast<float>(z) - static_cast<float>(blackRef.rawZ));

        // --- Step 2: Apply Color Correction Matrix ---
        // Matrix operates on compensated values to get LINEAR RGB
        float linearR = m[0][0] * compensatedX + m[0][1] * compensatedY + m[0][2] * compensatedZ;
        float linearG = m[1][0] * compensatedX + m[1][1] * compensatedY + m[1][2] * compensatedZ;
        float linearB = m[2][0] * compensatedX + m[2][1] * compensatedY + m[2][2] * compensatedZ;

        // --- Step 3: Normalize Linear Values ---
        // Prevent clipping by finding the maximum value and normalizing
        float maxLinear = fmax(1.0f, fmax(linearR / 255.0f, fmax(linearG / 255.0f, linearB / 255.0f)));

        float normR = fmax(0.0f, fmin(1.0f, (linearR / 255.0f) / maxLinear));
        float normG = fmax(0.0f, fmin(1.0f, (linearG / 255.0f) / maxLinear));
        float normB = fmax(0.0f, fmin(1.0f, (linearB / 255.0f) / maxLinear));

        // --- Step 4: Apply sRGB Gamma Correction ---
        // Convert from linear light space to sRGB gamma space
        float gammaR = applySRGBGamma(normR);
        float gammaG = applySRGBGamma(normG);
        float gammaB = applySRGBGamma(normB);

        // --- Step 5: Convert to 8-bit RGB ---
        r = static_cast<uint8_t>(gammaR * 255.0f);
        g = static_cast<uint8_t>(gammaG * 255.0f);
        b = static_cast<uint8_t>(gammaB * 255.0f);

        return true;
    }

private:
    /**
     * @brief Apply sRGB gamma correction (CRITICAL for proper color reproduction)
     * @param linear Linear RGB value (0.0-1.0)
     * @return Gamma-corrected value (0.0-1.0)
     */
    float applySRGBGamma(float linear) const {
        // Handle edge cases
        if (linear <= 0.0f) return 0.0f;
        if (linear >= 1.0f) return 1.0f;

        // sRGB gamma correction formula
        if (linear <= 0.0031308f) {
            return 12.92f * linear;  // Linear portion for very dark values
        } else {
            return 1.055f * pow(linear, 1.0f / 2.4f) - 0.055f;  // Gamma curve for normal values
        }
    }

public:
    // =============================================================================
    // BACKWARD COMPATIBILITY METHODS (DEPRECATED)
    // =============================================================================
    // These methods are provided for backward compatibility with existing code.
    // New code should use the unified apply() method with CompensationLevel parameters.

    /**
     * @brief Legacy apply method for backward compatibility
     * @deprecated Use apply() with CompensationLevel::NONE instead
     */
    bool apply(float x, float y, float z, uint8_t& r, uint8_t& g, uint8_t& b) const {
        // Convert normalized float inputs to uint16_t for unified method
        uint16_t rawX = static_cast<uint16_t>(x * 65535.0f);
        uint16_t rawY = static_cast<uint16_t>(y * 65535.0f);
        uint16_t rawZ = static_cast<uint16_t>(z * 65535.0f);

        return apply(rawX, rawY, rawZ, r, g, b, CompensationLevel::NONE);
    }

    /**
     * @brief Legacy applyProfessional method for backward compatibility
     * @deprecated Use apply() with CompensationLevel::PROFESSIONAL instead
     */
    bool applyProfessional(uint16_t x, uint16_t y, uint16_t z,
                          const CalibrationPoint& darkOffset,
                          const CalibrationPoint& blackRef,
                          uint8_t& r, uint8_t& g, uint8_t& b) const {
        return apply(x, y, z, r, g, b, CompensationLevel::PROFESSIONAL, &darkOffset, &blackRef);
    }

    /**
     * @brief Legacy applyWithBlackCompensation method for backward compatibility
     * @deprecated Use apply() with CompensationLevel::BLACK_ONLY instead
     */
    bool applyWithBlackCompensation(uint16_t x, uint16_t y, uint16_t z,
                                   const CalibrationPoint& blackRef,
                                   uint8_t& r, uint8_t& g, uint8_t& b) const {
        return apply(x, y, z, r, g, b, CompensationLevel::BLACK_ONLY, nullptr, &blackRef);
    }
};

/**
 * @brief Static target color definitions for calibration system
 *
 * Contains the exact RGB values for all reference colors used in the calibration system.
 * All colors are treated equally - there are no artificial "core," "legacy," or "extended"
 * categories since all valid calibration points contribute equally to matrix accuracy.
 */
struct TargetColors {
    // Special calibration points
    // Dark Offset (LED OFF, sensor covered) - True black
    static constexpr uint8_t DARK_OFFSET_R = 0;
    static constexpr uint8_t DARK_OFFSET_G = 0;
    static constexpr uint8_t DARK_OFFSET_B = 0;

    // Black reference color (LED ON, black swatch)
    static constexpr uint8_t BLACK_R = 5;
    static constexpr uint8_t BLACK_G = 5;
    static constexpr uint8_t BLACK_B = 5;

    // White reference color - "Vivid White"
    static constexpr uint8_t WHITE_R = 247;
    static constexpr uint8_t WHITE_G = 248;
    static constexpr uint8_t WHITE_B = 244;

    // Red reference color - Strong primary for matrix stability
    static constexpr uint8_t RED_R = 200;
    static constexpr uint8_t RED_G = 30;
    static constexpr uint8_t RED_B = 30;

    // Green reference color - Strong primary for matrix stability
    static constexpr uint8_t GREEN_R = 30;
    static constexpr uint8_t GREEN_G = 200;
    static constexpr uint8_t GREEN_B = 30;

    // Blue reference color - Strong primary for matrix stability
    static constexpr uint8_t BLUE_R = 30;
    static constexpr uint8_t BLUE_G = 30;
    static constexpr uint8_t BLUE_B = 200;

    // Yellow reference color
    static constexpr uint8_t YELLOW_R = 230;
    static constexpr uint8_t YELLOW_G = 220;
    static constexpr uint8_t YELLOW_B = 50;

    // REMOVED COLOR DEFINITIONS (no longer supported):
    // GREY_R/G/B, RED_REFERENCE_R/G/B, GREEN_REFERENCE_R/G/B,
    // HIGHGATE_R/G/B, DOMINO_R/G/B, TRANQUIL_RETREAT_R/G/B, GREY_CABIN_R/G/B
};

/**
 * @brief Calibration color enumeration - 6 Colors Only
 *
 * Defines the final 6 colors for the calibration system.
 * All colors are treated equally and contribute to matrix calculation accuracy.
 */
enum class CalibrationColor : uint8_t {
    BLACK = 0,      // Black reference (RGB: 5, 5, 5)
    WHITE = 1,      // Vivid White (RGB: 247, 248, 244) - replaces oversaturated white
    RED = 2,        // Red reference (RGB: 200, 30, 30)
    GREEN = 3,      // Green reference (RGB: 30, 200, 30)
    BLUE = 4,       // Blue reference (RGB: 30, 30, 200)
    YELLOW = 5,     // Yellow reference (RGB: 230, 220, 50)
    NONE = 6        // No color selected

    // REMOVED COLORS (no longer supported):
    // GREY = 2, HOG_BRISTLE = 5, HIGHGATE = 6, GREY_PORT = 7,
    // DOMINO = 8, TRANQUIL_RETREAT = 9, GREY_CABIN = 10
};

/**
 * @brief Auto-calibration state enumeration
 */
enum class AutoCalibrationState {
    IDLE,
    IN_PROGRESS,
    WAITING_FOR_SAMPLE,
    COMPLETED,
    CANCELLED
};

/**
 * @brief Calibration status tracking structure - 6 Colors Only
 *
 * Tracks the progress and status of the 6-color calibration process.
 */
struct CalibrationStatus {
    // Calibration status for 6 colors only
    bool blackCalibrated;   ///< Black reference calibrated
    bool whiteCalibrated;   ///< Vivid White calibrated (replaces oversaturated white)
    bool redCalibrated;     ///< Red reference calibrated
    bool greenCalibrated;   ///< Green reference calibrated
    bool blueCalibrated;    ///< Blue reference calibrated
    bool yellowCalibrated;  ///< Yellow reference calibrated

    // Overall status
    uint8_t totalPoints;            ///< Total calibration points
    uint8_t progress;               ///< Progress percentage (0-100)
    bool calibrationComplete;       ///< All required points calibrated
    bool ccmValid;                  ///< Color correction matrix valid

    /**
     * @brief Default constructor
     */
    CalibrationStatus() : blackCalibrated(false), whiteCalibrated(false),
                         redCalibrated(false), greenCalibrated(false),
                         blueCalibrated(false), yellowCalibrated(false),
                         totalPoints(0), progress(0),
                         calibrationComplete(false), ccmValid(false) {}

    // REMOVED FIELDS (colors no longer supported):
    // greyCalibrated, hogBristleCalibrated, highgateCalibrated, greyPortCalibrated,
    // dominoCalibrated, tranquilRetreatCalibrated, greyCabinCalibrated

    /**
     * @brief Get overall calibration progress for 6-color system
     * @return Progress percentage (0-100)
     */
    uint8_t getProgress() const {
        uint8_t count = 0;
        if (blackCalibrated) count++;
        if (whiteCalibrated) count++;
        if (redCalibrated) count++;
        if (greenCalibrated) count++;
        if (blueCalibrated) count++;
        if (yellowCalibrated) count++;
        return (count * 100) / 6; // 6 total colors
    }

    /**
     * @brief Check if minimum calibration is complete
     * @return true if at least black and vivid white are calibrated
     */
    bool isComplete() const {
        return blackCalibrated && whiteCalibrated;
    }
};

/**
 * @brief Auto-calibration status structure
 *
 * Tracks the state and progress of automated multi-color calibration
 */
struct AutoCalibrationStatus {
    AutoCalibrationState state;     ///< Current auto-calibration state
    CalibrationColor currentColor;  ///< Current color being calibrated
    uint8_t currentStep;            ///< Current step (1-12)
    uint8_t totalSteps;             ///< Total steps in auto-calibration
    String currentColorName;        ///< Human-readable color name
    uint8_t targetR;                ///< Target red value
    uint8_t targetG;                ///< Target green value
    uint8_t targetB;                ///< Target blue value
    uint8_t progress;               ///< Progress percentage (0-100)
    bool canSkip;                   ///< Whether current color can be skipped
    String instructions;            ///< Current step instructions
    bool isBlackStage1;             ///< True if in black calibration stage 1 (dark offset), false for stage 2 (black reference)
};

#endif // CALIBRATION_STRUCTURES_H
