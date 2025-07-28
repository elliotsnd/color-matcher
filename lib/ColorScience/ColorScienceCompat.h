#ifndef COLORSCIENCE_COMPAT_H
#define COLORSCIENCE_COMPAT_H

#include "ColorScience.h"
#include "../ColorCalibration/CalibrationStructures.h"

// Forward declarations for CIEDE2000 validation
class ColorScience;
extern float calculateColorDistance(uint8_t red1, uint8_t green1, uint8_t blue1,
                                   uint8_t red2, uint8_t green2, uint8_t blue2);

/**
 * @deprecated This structure is deprecated in favor of the unified calibration system
 * in CalibrationStructures.h. Use ColorCalibrationManager instead.
 *
 * Enhanced CalibrationData with Professional 4-Point Calibration Support
 *
 * ARCHITECTURAL DEBT WARNING: This structure creates a competing data structure
 * with CalibrationStructures.h, leading to two sources of truth for calibration state.
 *
 * MIGRATION PATH: Use the unified ColorCalibrationManager and CalibrationStatus
 * from CalibrationStructures.h instead of this deprecated structure.
 *
 * This structure extends the base CalibrationData with:
 * - 4-point calibration support (Black, White, Blue, Yellow)
 * - Professional status tracking
 * - Backward compatibility methods
 * - Direct field access compatibility
 *
 * Maintains full compatibility with existing ColorScience and TCS3430AutoGain libraries
 * while enabling professional-grade tetrahedral interpolation capabilities.
 */
struct EnhancedCalibrationData : public ColorScience::CalibrationData {
        
        // ========================================
        // Professional 4-Point Reference System
        // ========================================

        // Note: blueReference and yellowReference are inherited from ColorScience::CalibrationData
        
        // ========================================
        // Professional Status Tracking
        // ========================================
        
        /**
         * @deprecated Use CalibrationStatus from CalibrationStructures.h instead
         *
         * This Status struct creates architectural debt by duplicating functionality
         * from the unified CalibrationStatus. New code should use the unified system.
         */
        struct Status {
            bool blackComplete = false;      ///< Black reference calibration complete
            bool whiteComplete = false;      ///< White reference calibration complete
            bool blueComplete = false;       ///< Blue reference calibration complete
            bool yellowComplete = false;     ///< Yellow reference calibration complete

            /** Check if 2-point calibration is complete */
            bool is2PointCalibrated() const {
                return blackComplete && whiteComplete;
            }

            /** Check if full 4-point calibration is complete */
            bool is4PointCalibrated() const {
                return blackComplete && whiteComplete && blueComplete && yellowComplete;
            }

            /** Get calibration completeness percentage */
            float getCompletionPercentage() const {
                int completed = 0;
                if (blackComplete) completed++;
                if (whiteComplete) completed++;
                if (blueComplete) completed++;
                if (yellowComplete) completed++;
                return (completed / 4.0f) * 100.0f;
            }

            /** Reset all calibration status */
            void reset() {
                blackComplete = false;
                whiteComplete = false;
                blueComplete = false;
                yellowComplete = false;
            }

            /**
             * @brief Convert to unified CalibrationStatus
             * @return CalibrationStatus equivalent of this deprecated status
             */
            CalibrationStatus toUnifiedStatus() const {
                CalibrationStatus unified;
                unified.blackCalibrated = blackComplete;
                unified.whiteCalibrated = whiteComplete;
                unified.blueCalibrated = blueComplete;
                unified.yellowCalibrated = yellowComplete;
                // Other colors default to false in unified system
                return unified;
            }

            /**
             * @brief Update from unified CalibrationStatus
             * @param unified The unified status to copy from
             */
            void fromUnifiedStatus(const CalibrationStatus& unified) {
                blackComplete = unified.blackCalibrated;
                whiteComplete = unified.whiteCalibrated;
                blueComplete = unified.blueCalibrated;
                yellowComplete = unified.yellowCalibrated;
            }
        } status;
        
        // ========================================
        // Backward Compatibility Methods
        // ========================================
        
        /** Legacy compatibility: Check if calibrated */
        bool isCalibrated() const { 
            return status.is2PointCalibrated(); 
        }
        
        /** Legacy compatibility: Black reference complete */
        bool blackReferenceComplete() const { 
            return status.blackComplete; 
        }
        
        /** Legacy compatibility: White reference complete */
        bool whiteReferenceComplete() const { 
            return status.whiteComplete; 
        }
        
        // ========================================
        // Direct Field Access Compatibility
        // ========================================
        
        /** Direct access to black reference X (min) */
        float& minX() { return ColorScience::CalibrationData::blackReference.X; }
        const float& minX() const { return ColorScience::CalibrationData::blackReference.X; }

        /** Direct access to black reference Y (min) */
        float& minY() { return ColorScience::CalibrationData::blackReference.Y; }
        const float& minY() const { return ColorScience::CalibrationData::blackReference.Y; }

        /** Direct access to black reference Z (min) */
        float& minZ() { return ColorScience::CalibrationData::blackReference.Z; }
        const float& minZ() const { return ColorScience::CalibrationData::blackReference.Z; }

        /** Direct access to white reference X (max) */
        float& maxX() { return ColorScience::CalibrationData::whiteReference.X; }
        const float& maxX() const { return ColorScience::CalibrationData::whiteReference.X; }

        /** Direct access to white reference Y (max) */
        float& maxY() { return ColorScience::CalibrationData::whiteReference.Y; }
        const float& maxY() const { return ColorScience::CalibrationData::whiteReference.Y; }

        /** Direct access to white reference Z (max) */
        float& maxZ() { return ColorScience::CalibrationData::whiteReference.Z; }
        const float& maxZ() const { return ColorScience::CalibrationData::whiteReference.Z; }
        
        // ========================================
        // Professional Calibration Methods
        // ========================================
        
        /** Initialize with default professional values */
        void initializeProfessional() {
            // Initialize base calibration data
            *static_cast<ColorScience::CalibrationData*>(this) = ColorScience::createDefaultCalibration();

            // Note: blueReference and yellowReference are already initialized in the base class

            // Reset status
            status.reset();
        }
        
        /** Validate 4-point calibration data integrity */
        bool validate4PointCalibration() const {
            // Basic validation from base class
            if (!ColorScience::validateCalibrationData(*this)) {
                return false;
            }
            
            // 4-point specific validation
            if (status.is4PointCalibrated()) {
                // Ensure blue has Z-dominance
                float blueTotal = blueReference.X + blueReference.Y + blueReference.Z;
                if (blueTotal > 0.0f && (blueReference.Z / blueTotal) < 0.3f) {
                    return false;
                }

                // Ensure yellow has X+Y dominance
                float yellowTotal = yellowReference.X + yellowReference.Y + yellowReference.Z;
                if (yellowTotal > 0.0f && ((yellowReference.X + yellowReference.Y) / yellowTotal) < 0.5f) {
                    return false;
                }
            }
            
            return true;
        }
        
        /** Get calibration type string */
        const char* getCalibrationType() const {
            if (status.is4PointCalibrated()) {
                return "4-Point Professional";
            } else if (status.is2PointCalibrated()) {
                return "2-Point Standard";
            } else {
                return "Uncalibrated";
            }
        }
        
        /** Reset all calibration data */
        void resetCalibration() {
            // Reset base calibration using explicit base class access
            ColorScience::CalibrationData::whiteReference = {0.95047f, 1.00000f, 1.08883f};
            ColorScience::CalibrationData::blackReference = {0.0f, 0.0f, 0.0f};

            // Note: 4-point references are reset in the base class

            // Reset IR data if available
            if (ColorScience::CalibrationData::whiteReference.ir.IR1 != 0.0f ||
                ColorScience::CalibrationData::whiteReference.ir.IR2 != 0.0f) {
                ColorScience::CalibrationData::whiteReference.ir = {0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
                ColorScience::CalibrationData::blackReference.ir = {0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
            }

            // Reset status
            status.reset();
        }
        
        // ========================================
        // Professional Quality Metrics & CIEDE2000 Validation
        // ========================================
        
        /** Calculate calibration quality score (0-100) */
        float getQualityScore() const {
            if (!status.is2PointCalibrated()) {
                return 0.0f;
            }
            
            float score = 50.0f; // Base score for 2-point calibration
            
            // Add points for 4-point calibration
            if (status.is4PointCalibrated()) {
                score += 30.0f;
            }
            
            // Add points for good dynamic range
            float dynamicRange = (ColorScience::CalibrationData::whiteReference.Y -
                                  ColorScience::CalibrationData::blackReference.Y) /
                                  ColorScience::CalibrationData::whiteReference.Y;
            if (dynamicRange > 0.8f) {
                score += 20.0f;
            } else if (dynamicRange > 0.6f) {
                score += 10.0f;
            }
            
            return std::min(100.0f, score);
        }
        
        /** Get professional calibration summary */
        struct CalibrationSummary {
            const char* type;
            float completionPercentage;
            float qualityScore;
            bool isValid;
            const char* recommendation;
        };
        
        CalibrationSummary getSummary() const {
            CalibrationSummary summary;
            summary.type = getCalibrationType();
            summary.completionPercentage = status.getCompletionPercentage();
            summary.qualityScore = getQualityScore();
            summary.isValid = validate4PointCalibration();
            
            if (summary.qualityScore >= 90.0f) {
                summary.recommendation = "Excellent - Ready for critical applications";
            } else if (summary.qualityScore >= 70.0f) {
                summary.recommendation = "Good - Suitable for professional use";
            } else if (summary.qualityScore >= 50.0f) {
                summary.recommendation = "Acceptable - Consider recalibration for best results";
            } else {
                summary.recommendation = "Poor - Recalibration required";
            }
            
            return summary;
        }

        // ========================================
        // CIEDE2000 Professional Validation
        // ========================================

        /** Professional color accuracy validation using CIEDE2000 */
        struct CIEDE2000ValidationResult {
            bool passed;                    // Overall validation result
            float averageDeltaE;           // Average ΔE across test colors
            float maxDeltaE;               // Maximum ΔE found
            int testColorCount;            // Number of test colors used
            float professionalThreshold;   // ΔE threshold used (typically 2.0)
            const char* qualityLevel;      // Professional quality assessment
        };

        /**
         * Validate calibration accuracy using CIEDE2000 with professional test colors
         *
         * This method tests the calibration against known reference colors and measures
         * the color difference using the CIEDE2000 formula, which is the industry standard
         * for perceptual color difference measurement.
         *
         * Professional thresholds:
         * - ΔE < 1.0: Excellent (critical color matching)
         * - ΔE < 2.0: Good (professional applications)
         * - ΔE < 3.0: Acceptable (general use)
         * - ΔE > 3.0: Poor (recalibration needed)
         *
         * @param testColors Array of known RGB test colors
         * @param testCount Number of test colors
         * @param threshold Professional ΔE threshold (default: 2.0)
         * @return Detailed validation results
         */
        CIEDE2000ValidationResult validateWithCIEDE2000(const uint8_t testColors[][3],
                                                       int testCount,
                                                       float threshold = 2.0f) const;

        /**
         * Quick CIEDE2000 validation using standard test colors
         *
         * Uses a predefined set of professional test colors including:
         * - Primary colors (Red, Green, Blue)
         * - Secondary colors (Cyan, Magenta, Yellow)
         * - Neutral colors (White, Gray, Black)
         * - Skin tones and critical colors
         *
         * @param threshold Professional ΔE threshold (default: 2.0)
         * @return Validation result with professional assessment
         */
        CIEDE2000ValidationResult validateProfessionalAccuracy(float threshold = 2.0f) const;

        /**
         * Calculate CIEDE2000 color difference for a single color measurement
         *
         * @param measuredR Measured red component (0-255)
         * @param measuredG Measured green component (0-255)
         * @param measuredB Measured blue component (0-255)
         * @param expectedR Expected red component (0-255)
         * @param expectedG Expected green component (0-255)
         * @param expectedB Expected blue component (0-255)
         * @return CIEDE2000 ΔE value
         */
        float calculateColorDifference(uint8_t measuredR, uint8_t measuredG, uint8_t measuredB,
                                     uint8_t expectedR, uint8_t expectedG, uint8_t expectedB) const;
    };
    
    // ========================================
    // Utility Functions
    // ========================================
    
    /** Create enhanced calibration data with professional defaults */
    inline EnhancedCalibrationData createEnhancedCalibration() {
        EnhancedCalibrationData calibData;
        calibData.initializeProfessional();
        return calibData;
    }
    
    /**
     * @deprecated Convert legacy CalibrationData to EnhancedCalibrationData
     * Use ColorCalibrationManager instead for new code.
     */
    inline EnhancedCalibrationData upgradeCalibrationData(const ColorScience::CalibrationData& legacy) {
        EnhancedCalibrationData enhanced;

        // Copy base data
        static_cast<ColorScience::CalibrationData&>(enhanced) = legacy;

        // Initialize professional features
        // Note: blueReference and yellowReference are already initialized in the base class
        enhanced.status.reset();

        // Set status based on legacy data (if available)
        // This would need to be customized based on how legacy data indicates completion

        return enhanced;
    }

    // =============================================================================
    // MIGRATION GUIDE: RESOLVING COMPETING DATA STRUCTURES
    // =============================================================================

    /**
     * @brief Migration helper to convert from deprecated EnhancedCalibrationData to unified system
     *
     * ARCHITECTURAL DEBT RESOLUTION:
     * This file (ColorScienceCompat.h) creates competing data structures with CalibrationStructures.h,
     * leading to two sources of truth for calibration state. This is being resolved by:
     *
     * 1. DEPRECATING this entire file in favor of the unified system
     * 2. MIGRATING all code to use ColorCalibrationManager from CalibrationStructures.h
     * 3. PROVIDING migration helpers for backward compatibility during transition
     *
     * MIGRATION STEPS:
     * 1. Replace EnhancedCalibrationData with ColorCalibrationManager
     * 2. Replace EnhancedCalibrationData::Status with CalibrationStatus
     * 3. Use the unified apply() method with CompensationLevel instead of multiple methods
     * 4. Remove dependencies on this file once migration is complete
     *
     * BENEFITS OF UNIFIED SYSTEM:
     * - Single source of truth for calibration state
     * - Consistent API across all calibration operations
     * - Better error handling and fallback logic
     * - Extensible design for adding new colors
     * - Elimination of architectural debt
     */

    /**
     * @deprecated Use ColorCalibrationManager instead
     *
     * This function helps migrate from the deprecated EnhancedCalibrationData
     * to the unified ColorCalibrationManager system.
     */
    inline void migrateToUnifiedSystem(const EnhancedCalibrationData& deprecated) {
        // This would contain migration logic to transfer data to ColorCalibrationManager
        // Implementation depends on specific migration requirements

        // Example migration pattern:
        // ColorCalibrationManager& manager = ColorCalibration::getManager();
        // if (deprecated.status.blackComplete) {
        //     manager.addOrUpdateCalibrationPoint("black", ...);
        // }
        // ... etc for other colors
    }

#endif // COLORSCIENCE_COMPAT_H
