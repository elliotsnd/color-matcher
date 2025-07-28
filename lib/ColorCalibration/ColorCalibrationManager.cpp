/**
 * @file ColorCalibrationManager.cpp
 * @brief Implementation of main calibration manager
 * @author ESP32 Color Calibration System
 * @version 1.0.0
 * @date 2024
 * 
 * This file provides the complete implementation of the color calibration
 * manager, handling all aspects of the 5-point calibration system.
 */

#include "ColorCalibrationManager.h"
#include <Arduino.h>

ColorCalibrationManager::ColorCalibrationManager() : isInitialized(false), darkOffsetCalibrated(false), blackRefCalibrated(false) {
    lastError = "";

    // Initialize calibration points
    darkOffsetPoint = {0, 0, 0, 0, 0, 0, 0, 0.0f};
    blackRefPoint = {0, 0, 0, 0, 0, 0, 0, 0.0f};
}

ColorCalibrationManager::~ColorCalibrationManager() {
    // Nothing to clean up
}

bool ColorCalibrationManager::initialize() {
    if (isInitialized) {
        return true;
    }
    
    // Initialize preferences
    preferences.begin("color_cal", false);
    
    // Load existing calibration data
    bool loaded = loadCalibrationData();
    
    // Recalculate CCM if data was loaded
    if (loaded && !points.empty()) {
        recalculateCCM();
    }
    
    isInitialized = true;
    return true;
}

bool ColorCalibrationManager::addOrUpdateCalibrationPoint(const String& colorName, uint16_t rawX, uint16_t rawY, uint16_t rawZ, float quality) {
    // === ENHANCED ERROR HANDLING AND VALIDATION ===

    // Guard clause: Check initialization
    if (!isInitialized) {
        lastError = "Manager not initialized - call initialize() first";
        Serial.println("❌ ColorCalibrationManager: Not initialized");
        return false;
    }

    // Guard clause: Validate color name
    uint8_t targetR, targetG, targetB;
    if (!getTargetColor(colorName, targetR, targetG, targetB)) {
        lastError = "Invalid color name: " + colorName + " (supported: black, white, red, green, blue, grey, yellow, etc.)";
        Serial.println("❌ ColorCalibrationManager: Invalid color name: " + colorName);
        return false;
    }

    // Guard clause: Validate sensor readings (prevent overflow/underflow issues)
    if (rawX == 0 && rawY == 0 && rawZ == 0) {
        lastError = "Invalid sensor readings: all values are zero (sensor may be disconnected or covered)";
        Serial.println("❌ ColorCalibrationManager: Zero sensor readings detected");
        return false;
    }

    // Guard clause: Validate quality parameter
    if (quality < 0.0f || quality > 1.0f) {
        lastError = "Quality parameter out of range [0.0, 1.0]: " + String(quality);
        Serial.println("❌ ColorCalibrationManager: Invalid quality parameter: " + String(quality));
        return false;
    }

    // Guard clause: Check for sensor saturation (prevent matrix calculation issues)
    const uint16_t SATURATION_THRESHOLD = 65000; // Near max uint16_t
    if (rawX >= SATURATION_THRESHOLD || rawY >= SATURATION_THRESHOLD || rawZ >= SATURATION_THRESHOLD) {
        lastError = "Sensor saturation detected (X:" + String(rawX) + " Y:" + String(rawY) + " Z:" + String(rawZ) + ") - reduce LED brightness or integration time";
        Serial.println("⚠️ ColorCalibrationManager: Sensor saturation warning for " + colorName);
        // Don't return false - allow the point but warn user
    }

    Serial.println("✅ ColorCalibrationManager: Adding calibration point for " + colorName +
                   " XYZ(" + String(rawX) + "," + String(rawY) + "," + String(rawZ) + ") → RGB(" +
                   String(targetR) + "," + String(targetG) + "," + String(targetB) + ")");

    // Create new calibration point with validated data
    CalibrationPoint newPoint(rawX, rawY, rawZ, targetR, targetG, targetB, millis() / 1000, quality);
    
    // Find and update existing point or add new one
    bool found = false;
    for (auto& point : points) {
        if (point.targetR == targetR && point.targetG == targetG && point.targetB == targetB) {
            point = newPoint;
            found = true;
            break;
        }
    }
    if (!found) {
        points.push_back(newPoint);
    }
    
    // Recalculate CCM (this now handles failures gracefully)
    bool success = recalculateCCM();

    // Always save calibration data, even if matrix calculation failed
    // The points are still valuable for 2-point calibration
    saveCalibrationData();

    // Always return true - calibration point was successfully added
    // Matrix calculation failure is not a fatal error
    return true;
}

bool ColorCalibrationManager::calibrateDarkOffset(uint16_t rawX, uint16_t rawY, uint16_t rawZ) {
    if (!isInitialized) {
        lastError = "Manager not initialized";
        return false;
    }

    // Store the dark offset point (LED OFF reading)
    darkOffsetPoint = {rawX, rawY, rawZ, 0, 0, 0, millis() / 1000, 1.0f};
    darkOffsetCalibrated = true;

    // Save to persistent storage
    saveCalibrationData();

    lastError = "";
    return true;
}

bool ColorCalibrationManager::calibrateBlackReference(uint16_t rawX, uint16_t rawY, uint16_t rawZ) {
    if (!isInitialized) {
        lastError = "Manager not initialized";
        return false;
    }

    // Store the black reference point (LED ON with black sample)
    blackRefPoint = {rawX, rawY, rawZ, TargetColors::BLACK_R, TargetColors::BLACK_G, TargetColors::BLACK_B, millis() / 1000, 1.0f};
    blackRefCalibrated = true;

    // Also add this as a regular calibration point for matrix calculation
    CalibrationPoint newPoint(rawX, rawY, rawZ, TargetColors::BLACK_R, TargetColors::BLACK_G, TargetColors::BLACK_B, millis() / 1000, 1.0f);

    // Find and update existing black point or add new one
    bool found = false;
    for (auto& point : points) {
        if (point.targetR == TargetColors::BLACK_R && point.targetG == TargetColors::BLACK_G && point.targetB == TargetColors::BLACK_B) {
            point = newPoint;
            found = true;
            break;
        }
    }
    if (!found) {
        points.push_back(newPoint);
    }

    // Recalculate CCM
    recalculateCCM();

    // Save to persistent storage
    saveCalibrationData();

    lastError = "";
    return true;
}

bool ColorCalibrationManager::applyCalibrationCorrection(uint16_t rawX, uint16_t rawY, uint16_t rawZ, uint8_t& r, uint8_t& g, uint8_t& b) {
    // === ENHANCED ERROR HANDLING AND FALLBACK LOGIC ===

    // Guard clause: Check initialization with fallback
    if (!isInitialized) {
        Serial.println("⚠️ ColorCalibrationManager: Not initialized - using uncalibrated conversion (Tier 3)");
        normalizeXYZ(rawX, rawY, rawZ, r, g, b);
        return false;
    }

    // Guard clause: Validate input sensor readings
    if (rawX == 0 && rawY == 0 && rawZ == 0) {
        Serial.println("⚠️ ColorCalibrationManager: Zero sensor readings - using safe fallback values");
        r = g = b = 0; // Safe black output for zero input
        return false;
    }

    // Guard clause: Check for sensor overflow (prevent calculation errors)
    const uint16_t MAX_SAFE_VALUE = 65000;
    if (rawX > MAX_SAFE_VALUE || rawY > MAX_SAFE_VALUE || rawZ > MAX_SAFE_VALUE) {
        Serial.println("⚠️ ColorCalibrationManager: Sensor overflow detected - clamping values");
        // Clamp values to prevent overflow in calculations
        rawX = min(rawX, MAX_SAFE_VALUE);
        rawY = min(rawY, MAX_SAFE_VALUE);
        rawZ = min(rawZ, MAX_SAFE_VALUE);
    }

    // --- TIER 1: Matrix Calibration with Unified Apply Method ---
    // The unified apply method automatically selects the best compensation level
    // based on available calibration data (dark offset, black reference, etc.)
    if (isMatrixCalibrated() && points.size() >= 5) {
        // Prepare calibration points for unified method
        const CalibrationPoint* darkOffsetPtr = darkOffsetCalibrated ? &darkOffsetPoint : nullptr;
        const CalibrationPoint* blackRefPtr = blackRefCalibrated ? &blackRefPoint : nullptr;

        // Use AUTO compensation level - automatically selects best available pipeline:
        // - PROFESSIONAL: if both dark offset and black reference are available
        // - BLACK_ONLY: if only black reference is available
        // - NONE: if no compensation data is available (legacy compatibility)
        return ccm.apply(rawX, rawY, rawZ, r, g, b, CompensationLevel::AUTO, darkOffsetPtr, blackRefPtr);
    }

    // --- TIER 2: Good Quality (2-Point Calibration) ---
    // Maps to actual target RGB values to prevent over-saturation
    if (isTwoPointCalibrated()) {
        const CalibrationPoint* black = findPointByTarget(TargetColors::BLACK_R, TargetColors::BLACK_G, TargetColors::BLACK_B);
        const CalibrationPoint* white = findPointByTarget(TargetColors::WHITE_R, TargetColors::WHITE_G, TargetColors::WHITE_B);

        if (black && white) {
            // Map to actual target RGB values, not 0-255 to prevent over-saturation
            r = constrain(map(rawX, black->rawX, white->rawX, black->targetR, white->targetR), 0, 255);
            g = constrain(map(rawY, black->rawY, white->rawY, black->targetG, white->targetG), 0, 255);
            b = constrain(map(rawZ, black->rawZ, white->rawZ, black->targetB, white->targetB), 0, 255);
            return true; // Successfully applied 2-point calibration
        }
    }

    // --- TIER 3: Uncalibrated Fallback ---
    // Basic functionality - ensures device is always responsive
    r = static_cast<uint8_t>(constrain((float)rawX / 256.0f, 0.0f, 255.0f));
    g = static_cast<uint8_t>(constrain((float)rawY / 256.0f, 0.0f, 255.0f));
    b = static_cast<uint8_t>(constrain((float)rawZ / 256.0f, 0.0f, 255.0f));
    return false; // Indicate fallback was used
}

CalibrationStatus ColorCalibrationManager::getCalibrationStatus() const {
    CalibrationStatus status;
    
    for (const auto& point : points) {
        if (point.targetR == TargetColors::BLACK_R &&
            point.targetG == TargetColors::BLACK_G &&
            point.targetB == TargetColors::BLACK_B) {
            status.blackCalibrated = true;
        }

        if (point.targetR == TargetColors::WHITE_R &&
            point.targetG == TargetColors::WHITE_G &&
            point.targetB == TargetColors::WHITE_B) {
            status.whiteCalibrated = true;
        }

        if (point.targetR == TargetColors::RED_R &&
            point.targetG == TargetColors::RED_G &&
            point.targetB == TargetColors::RED_B) {
            status.redCalibrated = true;
        }

        if (point.targetR == TargetColors::GREEN_R &&
            point.targetG == TargetColors::GREEN_G &&
            point.targetB == TargetColors::GREEN_B) {
            status.greenCalibrated = true;
        }

        if (point.targetR == TargetColors::BLUE_R &&
            point.targetG == TargetColors::BLUE_G &&
            point.targetB == TargetColors::BLUE_B) {
            status.blueCalibrated = true;
        }

        if (point.targetR == TargetColors::YELLOW_R &&
            point.targetG == TargetColors::YELLOW_G &&
            point.targetB == TargetColors::YELLOW_B) {
            status.yellowCalibrated = true;
        }
    }
    
    return status;
}

ColorCorrectionMatrix ColorCalibrationManager::getColorCorrectionMatrix() const {
    return ccm;
}

bool ColorCalibrationManager::resetCalibration() {
    points.clear();
    ccm = ColorCorrectionMatrix();
    ccm.isValid = false;
    
    // Clear preferences
    preferences.clear();
    
    return true;
}

bool ColorCalibrationManager::saveCalibrationData() {
    if (!isInitialized) {
        return false;
    }

    // Save dark offset calibration data
    preferences.putBool("dark_offset_cal", darkOffsetCalibrated);
    if (darkOffsetCalibrated) {
        preferences.putUInt("dark_offset_x", darkOffsetPoint.rawX);
        preferences.putUInt("dark_offset_y", darkOffsetPoint.rawY);
        preferences.putUInt("dark_offset_z", darkOffsetPoint.rawZ);
        preferences.putUInt("dark_offset_ts", darkOffsetPoint.timestamp);
        preferences.putFloat("dark_offset_q", darkOffsetPoint.quality);
    }

    // Save black reference calibration data
    preferences.putBool("black_ref_cal", blackRefCalibrated);
    if (blackRefCalibrated) {
        preferences.putUInt("black_ref_x", blackRefPoint.rawX);
        preferences.putUInt("black_ref_y", blackRefPoint.rawY);
        preferences.putUInt("black_ref_z", blackRefPoint.rawZ);
        preferences.putUChar("black_ref_r", blackRefPoint.targetR);
        preferences.putUChar("black_ref_g", blackRefPoint.targetG);
        preferences.putUChar("black_ref_b", blackRefPoint.targetB);
        preferences.putUInt("black_ref_ts", blackRefPoint.timestamp);
        preferences.putFloat("black_ref_q", blackRefPoint.quality);
    }

    // Save number of points
    preferences.putUInt("num_points", points.size());

    // Save each point
    for (size_t i = 0; i < points.size(); i++) {
        String prefix = "point_" + String(i) + "_";
        preferences.putUInt((prefix + "x").c_str(), points[i].rawX);
        preferences.putUInt((prefix + "y").c_str(), points[i].rawY);
        preferences.putUInt((prefix + "z").c_str(), points[i].rawZ);
        preferences.putUChar((prefix + "r").c_str(), points[i].targetR);
        preferences.putUChar((prefix + "g").c_str(), points[i].targetG);
        preferences.putUChar((prefix + "b").c_str(), points[i].targetB);
        preferences.putUInt((prefix + "ts").c_str(), points[i].timestamp);
        preferences.putFloat((prefix + "quality").c_str(), points[i].quality);
    }

    return true;
}

bool ColorCalibrationManager::loadCalibrationData() {
    if (!isInitialized) {
        return false;
    }

    points.clear();

    // Load dark offset calibration data
    darkOffsetCalibrated = preferences.getBool("dark_offset_cal", false);
    if (darkOffsetCalibrated) {
        darkOffsetPoint.rawX = preferences.getUInt("dark_offset_x", 0);
        darkOffsetPoint.rawY = preferences.getUInt("dark_offset_y", 0);
        darkOffsetPoint.rawZ = preferences.getUInt("dark_offset_z", 0);
        darkOffsetPoint.targetR = 0;
        darkOffsetPoint.targetG = 0;
        darkOffsetPoint.targetB = 0;
        darkOffsetPoint.timestamp = preferences.getUInt("dark_offset_ts", 0);
        darkOffsetPoint.quality = preferences.getFloat("dark_offset_q", 1.0f);
    }

    // Load black reference calibration data
    blackRefCalibrated = preferences.getBool("black_ref_cal", false);
    if (blackRefCalibrated) {
        blackRefPoint.rawX = preferences.getUInt("black_ref_x", 0);
        blackRefPoint.rawY = preferences.getUInt("black_ref_y", 0);
        blackRefPoint.rawZ = preferences.getUInt("black_ref_z", 0);
        blackRefPoint.targetR = preferences.getUChar("black_ref_r", TargetColors::BLACK_R);
        blackRefPoint.targetG = preferences.getUChar("black_ref_g", TargetColors::BLACK_G);
        blackRefPoint.targetB = preferences.getUChar("black_ref_b", TargetColors::BLACK_B);
        blackRefPoint.timestamp = preferences.getUInt("black_ref_ts", 0);
        blackRefPoint.quality = preferences.getFloat("black_ref_q", 1.0f);
    }

    // Load number of points
    uint32_t numPoints = preferences.getUInt("num_points", 0);

    // Load each point
    for (uint32_t i = 0; i < numPoints; i++) {
        String prefix = "point_" + String(i) + "_";

        CalibrationPoint point;
        point.rawX = preferences.getUInt((prefix + "x").c_str(), 0);
        point.rawY = preferences.getUInt((prefix + "y").c_str(), 0);
        point.rawZ = preferences.getUInt((prefix + "z").c_str(), 0);
        point.targetR = preferences.getUChar((prefix + "r").c_str(), 0);
        point.targetG = preferences.getUChar((prefix + "g").c_str(), 0);
        point.targetB = preferences.getUChar((prefix + "b").c_str(), 0);
        point.timestamp = preferences.getUInt((prefix + "ts").c_str(), 0);
        point.quality = preferences.getFloat((prefix + "quality").c_str(), 1.0f);

        points.push_back(point);
    }

    return true;
}

std::vector<CalibrationPoint> ColorCalibrationManager::getCalibrationPoints() const {
    return points;
}

bool ColorCalibrationManager::isTwoPointCalibrated() const {
    const CalibrationPoint* black = findPointByTarget(TargetColors::BLACK_R, TargetColors::BLACK_G, TargetColors::BLACK_B);
    const CalibrationPoint* white = findPointByTarget(TargetColors::WHITE_R, TargetColors::WHITE_G, TargetColors::WHITE_B);
    return (black != nullptr) && (white != nullptr);
}

bool ColorCalibrationManager::isMatrixCalibrated() const {
    return ccm.isValid && (points.size() >= 5);
}

const CalibrationPoint* ColorCalibrationManager::findPointByTarget(uint8_t targetR, uint8_t targetG, uint8_t targetB) const {
    for (const auto& point : points) {
        if (point.targetR == targetR && point.targetG == targetG && point.targetB == targetB) {
            return &point;
        }
    }
    return nullptr;
}

bool ColorCalibrationManager::recalculateCCM() {
    if (points.empty()) {
        ccm.isValid = false;
        Serial.println("❌ CCM: No calibration points available");
        return false;
    }

    // --- ENHANCED LOGGING AND VALIDATION ---
    Serial.println("=== CCM CALCULATION ATTEMPT ===");
    Serial.println("📊 Available calibration points: " + String(points.size()));

    // List all available points for debugging
    for (size_t i = 0; i < points.size(); i++) {
        Serial.println("   Point " + String(i+1) + ": XYZ(" + String(points[i].rawX) + "," +
                      String(points[i].rawY) + "," + String(points[i].rawZ) + ") → RGB(" +
                      String(points[i].targetR) + "," + String(points[i].targetG) + "," + String(points[i].targetB) + ")");
    }

    // Use 5 points as minimum for truly robust matrix calculation
    // This ensures we have the full 5-point calibration before attempting matrix
    if (points.size() < 5) {
        ccm.isValid = false;
        Serial.println("⚠️  CCM: Need " + String(5 - points.size()) + " more point(s) for 5-point matrix calculation");
        Serial.println("   System will continue using 2-point calibration (Tier 2)");
        // This is not an error - just not enough points yet for robust matrix
        // The system will continue using 2-point calibration which is reliable
        return true; // Return true to indicate successful handling
    }

    Serial.println("🔄 Attempting 5-point CCM calculation...");

    // Attempt matrix calculation with full validation
    bool success = solver.calculateCCM(points, ccm);

    if (!success) {
        lastError = solver.getLastError();
        ccm.isValid = false;
        Serial.println("❌ CCM CALCULATION FAILED!");
        Serial.println("   Reason: " + lastError);
        Serial.println("   System will fall back to 2-point calibration (Tier 2)");
        Serial.println("   Suggestion: Ensure calibration points are diverse in color");
        // Even if matrix calculation fails, this is not a fatal error
        // The system will fall back to 2-point calibration
        return true; // Return true to indicate graceful handling
    }

    Serial.println("✅ CCM CALCULATION SUCCESS!");
    Serial.println("   5-point matrix is now ACTIVE (Tier 1 Professional Quality)");
    Serial.println("   System should now provide <2 RGB point accuracy");

    return success;
}

bool ColorCalibrationManager::getTargetColor(const String& colorName, uint8_t& r, uint8_t& g, uint8_t& b) {
    // Core 6-point mathematically robust calibration colors
    if (colorName == "dark_offset") {
        r = TargetColors::DARK_OFFSET_R;
        g = TargetColors::DARK_OFFSET_G;
        b = TargetColors::DARK_OFFSET_B;
        return true;
    }

    if (colorName == "black") {
        r = TargetColors::BLACK_R;
        g = TargetColors::BLACK_G;
        b = TargetColors::BLACK_B;
        return true;
    }

    if (colorName == "white") {
        r = TargetColors::WHITE_R;
        g = TargetColors::WHITE_G;
        b = TargetColors::WHITE_B;
        return true;
    }

    if (colorName == "red") {
        r = TargetColors::RED_R;
        g = TargetColors::RED_G;
        b = TargetColors::RED_B;
        return true;
    }

    if (colorName == "green") {
        r = TargetColors::GREEN_R;
        g = TargetColors::GREEN_G;
        b = TargetColors::GREEN_B;
        return true;
    }

    if (colorName == "blue") {
        r = TargetColors::BLUE_R;
        g = TargetColors::BLUE_G;
        b = TargetColors::BLUE_B;
        return true;
    }

    // REMOVED: Legacy grey color (no longer supported in 6-color system)

    if (colorName == "yellow") {
        r = TargetColors::YELLOW_R;
        g = TargetColors::YELLOW_G;
        b = TargetColors::YELLOW_B;
        return true;
    }

    // REMOVED: Extended calibration colors (no longer supported in 6-color system)
    // All removed colors: grey, red reference, highgate, green reference, domino, tranquil retreat, grey cabin

    return false;
}



// Helper function for fallback conversion
void ColorCalibrationManager::normalizeXYZ(uint16_t rawX, uint16_t rawY, uint16_t rawZ, uint8_t& r, uint8_t& g, uint8_t& b) {
    // Simple fallback: normalize XYZ to RGB
    float sum = rawX + rawY + rawZ;
    if (sum > 0) {
        r = (uint8_t)constrain((int)((rawX / sum) * 255), 0, 255);
        g = (uint8_t)constrain((int)((rawY / sum) * 255), 0, 255);
        b = (uint8_t)constrain((int)((rawZ / sum) * 255), 0, 255);
    } else {
        r = g = b = 0;
    }
}

// Auto-calibration implementation with 6-color system only
bool ColorCalibrationManager::startAutoCalibration() {
    // Initialize auto-calibration sequence (6 colors only)
    autoCalSequence = {
        CalibrationColor::BLACK,        // Step 1: Black reference
        CalibrationColor::WHITE,        // Step 2: Vivid White (mapped to WHITE enum)
        CalibrationColor::RED,          // Step 3: Red reference
        CalibrationColor::GREEN,        // Step 4: Green reference
        CalibrationColor::BLUE,         // Step 5: Blue reference
        CalibrationColor::YELLOW        // Step 6: Yellow reference
        // REMOVED: GREY, HOG_BRISTLE, HIGHGATE, GREY_PORT, DOMINO, TRANQUIL_RETREAT, GREY_CABIN
    };

    // Initialize auto-calibration status
    autoCalStatus.state = AutoCalibrationState::IN_PROGRESS;
    autoCalStatus.currentStep = 1;
    autoCalStatus.totalSteps = autoCalSequence.size();
    autoCalStatus.currentColor = autoCalSequence[0];
    autoCalStatus.progress = 0;
    autoCalStatus.canSkip = false; // Black cannot be skipped
    autoCalStatus.isBlackStage1 = true; // Start with dark offset stage

    // Set current color info for black calibration stage 1 (dark offset)
    String colorName;
    uint8_t r, g, b;
    if (getColorInfo(autoCalStatus.currentColor, colorName, r, g, b)) {
        autoCalStatus.currentColorName = colorName;
        autoCalStatus.targetR = r;
        autoCalStatus.targetG = g;
        autoCalStatus.targetB = b;
        autoCalStatus.instructions = "STAGE 1: Cover sensor completely to block all light. LED will turn OFF automatically for dark offset measurement.";
    }

    return true;
}

AutoCalibrationStatus ColorCalibrationManager::getAutoCalibrationStatus() const {
    return autoCalStatus;
}

bool ColorCalibrationManager::autoCalibrationNext() {
    if (autoCalStatus.state != AutoCalibrationState::IN_PROGRESS) {
        return false;
    }

    // Handle special two-stage black calibration
    if (autoCalStatus.currentColor == CalibrationColor::BLACK) {
        if (autoCalStatus.isBlackStage1) {
            // Stage 1: Dark offset calibration (LED OFF)
            if (performDarkOffsetCalibration()) {
                // Move to stage 2: Black reference calibration (LED ON)
                autoCalStatus.isBlackStage1 = false;
                autoCalStatus.instructions = "STAGE 2: Place BLACK sample over sensor. LED is now ON for black reference measurement.";
                return true;
            } else {
                autoCalStatus.instructions = "STAGE 1 FAILED: Please cover sensor completely and try again.";
                return false;
            }
        } else {
            // Stage 2: Black reference calibration (LED ON)
            if (performBlackReferenceCalibration()) {
                // Black calibration complete, advance to next color
                autoCalStatus.currentStep++;
                autoCalStatus.progress = (autoCalStatus.currentStep - 1) * 100 / autoCalStatus.totalSteps;

                if (autoCalStatus.currentStep > autoCalStatus.totalSteps) {
                    autoCalStatus.state = AutoCalibrationState::COMPLETED;
                    autoCalStatus.instructions = "Auto-calibration completed successfully!";

                    // SAFETY: Ensure LED is restored after completion
                    extern uint8_t getCurrentLedBrightness();
                    extern bool setHardwareLedBrightness(uint8_t brightness);
                    uint8_t currentBrightness = getCurrentLedBrightness();
                    if (currentBrightness == 0) {
                        setHardwareLedBrightness(128); // Restore to default
                        Serial.println("🔆 Auto-calibration complete: LED restored to default brightness");
                    }

                    return true;
                }

                // Set next color
                autoCalStatus.currentColor = autoCalSequence[autoCalStatus.currentStep - 1];
                autoCalStatus.canSkip = (autoCalStatus.currentColor != CalibrationColor::BLACK &&
                                        autoCalStatus.currentColor != CalibrationColor::WHITE);

                // Set current color info
                String colorName;
                uint8_t r, g, b;
                if (getColorInfo(autoCalStatus.currentColor, colorName, r, g, b)) {
                    autoCalStatus.currentColorName = colorName;
                    autoCalStatus.targetR = r;
                    autoCalStatus.targetG = g;
                    autoCalStatus.targetB = b;
                    autoCalStatus.instructions = "Place " + colorName + " sample over sensor and click Next";
                }
                return true;
            } else {
                autoCalStatus.instructions = "STAGE 2 FAILED: Please place BLACK sample over sensor and try again.";
                return false;
            }
        }
    }

    // Handle normal calibration for other colors
    if (performNormalCalibration()) {
        autoCalStatus.currentStep++;
        autoCalStatus.progress = (autoCalStatus.currentStep - 1) * 100 / autoCalStatus.totalSteps;

        if (autoCalStatus.currentStep > autoCalStatus.totalSteps) {
            autoCalStatus.state = AutoCalibrationState::COMPLETED;
            autoCalStatus.instructions = "Auto-calibration completed successfully!";

            // SAFETY: Ensure LED is restored after completion
            extern uint8_t getCurrentLedBrightness();
            extern bool setHardwareLedBrightness(uint8_t brightness);
            uint8_t currentBrightness = getCurrentLedBrightness();
            if (currentBrightness == 0) {
                setHardwareLedBrightness(128); // Restore to default
                Serial.println("🔆 Auto-calibration complete: LED restored to default brightness");
            }

            return true;
        }

        // Set next color
        autoCalStatus.currentColor = autoCalSequence[autoCalStatus.currentStep - 1];
        autoCalStatus.canSkip = (autoCalStatus.currentColor != CalibrationColor::BLACK &&
                                autoCalStatus.currentColor != CalibrationColor::WHITE);

        // Set current color info
        String colorName;
        uint8_t r, g, b;
        if (getColorInfo(autoCalStatus.currentColor, colorName, r, g, b)) {
            autoCalStatus.currentColorName = colorName;
            autoCalStatus.targetR = r;
            autoCalStatus.targetG = g;
            autoCalStatus.targetB = b;
            autoCalStatus.instructions = "Place " + colorName + " sample over sensor and click Next";
        }
        return true;
    }

    return false;
}

bool ColorCalibrationManager::autoCalibrationRetry() {
    if (autoCalStatus.state != AutoCalibrationState::IN_PROGRESS) {
        return false;
    }

    // Just update instructions for retry
    autoCalStatus.instructions = "Retry: Place " + autoCalStatus.currentColorName + " sample over sensor and click Next";
    return true;
}

bool ColorCalibrationManager::autoCalibrationSkip() {
    if (autoCalStatus.state != AutoCalibrationState::IN_PROGRESS || !autoCalStatus.canSkip) {
        return false;
    }

    // Skip current color and advance
    return autoCalibrationNext();
}

bool ColorCalibrationManager::autoCalibrationComplete() {
    autoCalStatus.state = AutoCalibrationState::COMPLETED;
    autoCalStatus.progress = 100;
    autoCalStatus.instructions = "Auto-calibration completed!";

    // SAFETY: Ensure LED is restored to a reasonable brightness after calibration
    extern uint8_t getCurrentLedBrightness();
    extern bool setHardwareLedBrightness(uint8_t brightness);

    uint8_t currentBrightness = getCurrentLedBrightness();
    if (currentBrightness == 0) {
        // LED is off, restore to default brightness
        setHardwareLedBrightness(128); // Default medium brightness
        Serial.println("🔆 Auto-calibration complete: LED restored to default brightness (128)");
    } else {
        Serial.println("🔆 Auto-calibration complete: LED brightness is " + String(currentBrightness));
    }

    return true;
}

bool ColorCalibrationManager::getColorInfo(CalibrationColor color, String& name, uint8_t& r, uint8_t& g, uint8_t& b) const {
    switch (color) {
        case CalibrationColor::BLACK:
            name = "Black";
            r = TargetColors::BLACK_R;
            g = TargetColors::BLACK_G;
            b = TargetColors::BLACK_B;
            return true;
        case CalibrationColor::WHITE:
            name = "Vivid White";
            r = TargetColors::WHITE_R;
            g = TargetColors::WHITE_G;
            b = TargetColors::WHITE_B;
            return true;
        case CalibrationColor::RED:
            name = "Red";
            r = TargetColors::RED_R;
            g = TargetColors::RED_G;
            b = TargetColors::RED_B;
            return true;
        case CalibrationColor::GREEN:
            name = "Green";
            r = TargetColors::GREEN_R;
            g = TargetColors::GREEN_G;
            b = TargetColors::GREEN_B;
            return true;
        case CalibrationColor::BLUE:
            name = "Blue";
            r = TargetColors::BLUE_R;
            g = TargetColors::BLUE_G;
            b = TargetColors::BLUE_B;
            return true;
        case CalibrationColor::YELLOW:
            name = "Yellow";
            r = TargetColors::YELLOW_R;
            g = TargetColors::YELLOW_G;
            b = TargetColors::YELLOW_B;
            return true;

        // REMOVED COLORS (no longer supported):
        // GREY, HOG_BRISTLE, HIGHGATE, GREY_PORT, DOMINO, TRANQUIL_RETREAT, GREY_CABIN
        default:
            return false;
    }
}

// Auto-calibration helper functions
bool ColorCalibrationManager::performDarkOffsetCalibration() {
    Serial.println("=== DARK OFFSET CALIBRATION (LED OFF) ===");

    // Get current LED brightness to restore later
    extern uint8_t getCurrentLedBrightness();
    extern bool setHardwareLedBrightness(uint8_t brightness);
    extern bool readHardwareSensorAveraged(uint16_t& x, uint16_t& y, uint16_t& z);

    uint8_t originalBrightness = getCurrentLedBrightness();

    // Step 1: Turn LED OFF
    if (!setHardwareLedBrightness(0)) {
        Serial.println("❌ Failed to turn LED OFF");
        return false;
    }

    Serial.println("LED turned OFF for dark offset measurement");

    // Step 2: Wait for stabilization
    delay(500);

    // Step 3: Take averaged sensor readings
    uint16_t darkX, darkY, darkZ;
    if (!readHardwareSensorAveraged(darkX, darkY, darkZ)) {
        Serial.println("❌ Failed to read sensor for dark offset");
        setHardwareLedBrightness(originalBrightness); // Restore LED
        return false;
    }

    // Step 4: Restore LED brightness
    setHardwareLedBrightness(originalBrightness);
    Serial.println("LED restored to original brightness: " + String(originalBrightness));

    // Step 5: Store the dark offset
    bool success = calibrateDarkOffset(darkX, darkY, darkZ);

    if (success) {
        Serial.println("✅ Dark offset calibration successful: X=" + String(darkX) + " Y=" + String(darkY) + " Z=" + String(darkZ));
    } else {
        Serial.println("❌ Dark offset calibration failed");
    }

    return success;
}

bool ColorCalibrationManager::performBlackReferenceCalibration() {
    Serial.println("=== BLACK REFERENCE CALIBRATION (LED ON) ===");

    extern bool readHardwareSensorAveraged(uint16_t& x, uint16_t& y, uint16_t& z);

    // Step 1: Ensure LED is ON (should already be restored from dark offset calibration)
    Serial.println("Taking black reference readings with LED ON and black sample");

    // Step 2: Take averaged sensor readings with black sample
    uint16_t blackX, blackY, blackZ;
    if (!readHardwareSensorAveraged(blackX, blackY, blackZ)) {
        Serial.println("❌ Failed to read sensor for black reference");
        return false;
    }

    // Step 3: Store the black reference
    bool success = calibrateBlackReference(blackX, blackY, blackZ);

    if (success) {
        Serial.println("✅ Black reference calibration successful: X=" + String(blackX) + " Y=" + String(blackY) + " Z=" + String(blackZ));

        // Step 4: Also add to calibration points for matrix calculation
        String colorName = "black"; // Use internal name directly
        bool addSuccess = addOrUpdateCalibrationPoint(colorName, blackX, blackY, blackZ);

        if (addSuccess) {
            Serial.println("✅ Black point added to 5-point calibration matrix");
        } else {
            Serial.println("⚠️ Black reference stored but failed to add to matrix points");
        }
    } else {
        Serial.println("❌ Black reference calibration failed");
    }

    return success;
}

bool ColorCalibrationManager::performNormalCalibration() {
    extern bool readHardwareSensorAveraged(uint16_t& x, uint16_t& y, uint16_t& z);

    // Get current color info
    String colorName;
    uint8_t targetR, targetG, targetB;
    if (!getColorInfo(autoCalStatus.currentColor, colorName, targetR, targetG, targetB)) {
        Serial.println("❌ Failed to get color info for current calibration step");
        return false;
    }

    String upperColorName = colorName;
    upperColorName.toUpperCase();
    Serial.println("=== " + upperColorName + " CALIBRATION ===");
    Serial.println("Target RGB: (" + String(targetR) + "," + String(targetG) + "," + String(targetB) + ")");

    // Take averaged sensor readings for current color
    uint16_t sensorX, sensorY, sensorZ;
    if (!readHardwareSensorAveraged(sensorX, sensorY, sensorZ)) {
        Serial.println("❌ Failed to read sensor for " + colorName + " calibration");
        return false;
    }

    // Add to calibration points (convert display name to internal name)
    String internalColorName = colorName;
    internalColorName.toLowerCase();

    // Direct mappings for display names to internal names
    if (internalColorName == "vivid white") {
        internalColorName = "white";
    } else if (internalColorName == "timeless grey") {
        internalColorName = "grey";
    }
    // Note: Other colors like "Red Reference", "Green Reference", "Highgate",
    // "Domino", "Tranquil Retreat", "Grey Cabin" are now supported directly
    // in getTargetColor, so no conversion needed

    bool success = addOrUpdateCalibrationPoint(internalColorName, sensorX, sensorY, sensorZ);

    if (success) {
        Serial.println("✅ " + colorName + " calibration successful: X=" + String(sensorX) + " Y=" + String(sensorY) + " Z=" + String(sensorZ));

        // Check if we now have enough points for matrix calculation
        if (points.size() >= 5) {
            Serial.println("🎉 5-point calibration complete! Matrix calculation should be available.");
        } else {
            Serial.println("📊 Calibration progress: " + String(points.size()) + "/5 points collected");
        }
    } else {
        Serial.println("❌ " + colorName + " calibration failed");
    }

    return success;
}
