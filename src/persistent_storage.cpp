#include "persistent_storage.h"
#include "sensor_settings.h"
#include <time.h>

// Forward declarations for time functions
extern uint32_t getCurrentTimestamp();
extern String getCurrentTimeString();

// Global storage instance
PersistentStorage persistentStorage;

PersistentStorage::PersistentStorage() : initialized(false), currentCaptureIndex(0), totalCaptures(0) {
}

PersistentStorage::~PersistentStorage() {
    end();
}

bool PersistentStorage::begin() {
    if (initialized) {
        return true;
    }
    
    Serial.println("=== Initializing Persistent Storage ===");
    
    // Load current state from flash
    if (openNamespace(CAPTURES_NAMESPACE, true)) {
        totalCaptures = preferences.getUChar("totalCaptures", 0);
        currentCaptureIndex = preferences.getUChar("currentIndex", 0);
        closeNamespace();
        
        // Validate loaded values
        if (totalCaptures > MAX_COLOR_CAPTURES) {
            Serial.println("Warning: Invalid totalCaptures, resetting to 0");
            totalCaptures = 0;
        }
        if (currentCaptureIndex >= MAX_COLOR_CAPTURES) {
            Serial.println("Warning: Invalid currentIndex, resetting to 0");
            currentCaptureIndex = 0;
        }
    }
    
    // Load calibration data
    loadCalibrationData(calibrationData);
    
    initialized = true;
    Serial.printf("Storage initialized: %d captures, index %d\n", totalCaptures, currentCaptureIndex);
    
    return true;
}

void PersistentStorage::end() {
    if (initialized) {
        initialized = false;
        Serial.println("Persistent storage closed");
    }
}

bool PersistentStorage::openNamespace(const char* namespaceName, bool readOnly) {
    return preferences.begin(namespaceName, readOnly);
}

void PersistentStorage::closeNamespace() {
    preferences.end();
}

String PersistentStorage::generateCaptureKey(uint8_t index) {
    return "capture_" + String(index);
}

bool PersistentStorage::validateCaptureData(const StoredColorCapture& capture) {
    // Basic validation
    if (!capture.isValid) return false;
    if (capture.timestamp == 0) return false;
    if (strlen(capture.colorName) == 0) return false;
    
    // Reasonable sensor value ranges
    if (capture.x > 65535 || capture.y > 65535 || capture.z > 65535) return false;
    if (capture.ir1 > 65535 || capture.ir2 > 65535) return false;
    
    // RGB values should be 0-255
    if (capture.r > 255 || capture.g > 255 || capture.b > 255) return false;
    
    return true;
}

bool PersistentStorage::validateCalibrationData(const StoredCalibrationData& calibData) {
    // Must have at least black and white references for basic calibration
    if (!calibData.blackReference.isValid || !calibData.whiteReference.isValid) {
        return false;
    }
    
    // Timestamps should be reasonable
    if (calibData.blackReference.timestamp == 0 || calibData.whiteReference.timestamp == 0) {
        return false;
    }
    
    // Quality values should be between 0 and 1
    if (calibData.blackReference.quality < 0.0f || calibData.blackReference.quality > 1.0f) {
        return false;
    }
    if (calibData.whiteReference.quality < 0.0f || calibData.whiteReference.quality > 1.0f) {
        return false;
    }
    
    // FIXED: Validate blue reference if present (blue/yellow calibration now works!)
    if (calibData.blueComplete && calibData.blueReference.isValid) {
        if (calibData.blueReference.timestamp == 0) {
            return false;
        }
        if (calibData.blueReference.quality < 0.0f || calibData.blueReference.quality > 1.0f) {
            return false;
        }
    }
    
    // FIXED: Validate yellow reference if present (blue/yellow calibration now works!)
    if (calibData.yellowComplete && calibData.yellowReference.isValid) {
        if (calibData.yellowReference.timestamp == 0) {
            return false;
        }
        if (calibData.yellowReference.quality < 0.0f || calibData.yellowReference.quality > 1.0f) {
            return false;
        }
    }
    
    return true;
}

bool PersistentStorage::saveColorCapture(const StoredColorCapture& capture) {
    if (!initialized) {
        Serial.println("Error: Storage not initialized");
        return false;
    }
    
    if (!validateCaptureData(capture)) {
        Serial.println("Error: Invalid capture data");
        return false;
    }
    
    if (!openNamespace(CAPTURES_NAMESPACE, false)) {
        Serial.println("Error: Failed to open captures namespace");
        return false;
    }
    
    // Use circular buffer - overwrite oldest if full
    uint8_t saveIndex = currentCaptureIndex;
    String key = generateCaptureKey(saveIndex);
    
    // Save the capture data
    size_t bytesWritten = preferences.putBytes(key.c_str(), &capture, sizeof(StoredColorCapture));
    
    if (bytesWritten != sizeof(StoredColorCapture)) {
        Serial.printf("Error: Failed to save capture %d\n", saveIndex);
        closeNamespace();
        return false;
    }
    
    // Update indices
    currentCaptureIndex = (currentCaptureIndex + 1) % MAX_COLOR_CAPTURES;
    if (totalCaptures < MAX_COLOR_CAPTURES) {
        totalCaptures++;
    }
    
    // Save metadata
    preferences.putUChar("totalCaptures", totalCaptures);
    preferences.putUChar("currentIndex", currentCaptureIndex);
    
    closeNamespace();
    
    Serial.printf("Saved capture %d: %s (R:%d G:%d B:%d)\n", 
                  saveIndex, capture.colorName, capture.r, capture.g, capture.b);
    
    return true;
}

bool PersistentStorage::loadColorCapture(uint8_t index, StoredColorCapture& capture) {
    if (!initialized) {
        Serial.println("Error: Storage not initialized");
        return false;
    }
    
    if (index >= MAX_COLOR_CAPTURES) {
        Serial.printf("Error: Invalid capture index %d\n", index);
        return false;
    }
    
    if (!openNamespace(CAPTURES_NAMESPACE, true)) {
        Serial.println("Error: Failed to open captures namespace");
        return false;
    }
    
    String key = generateCaptureKey(index);
    size_t bytesRead = preferences.getBytes(key.c_str(), &capture, sizeof(StoredColorCapture));
    
    closeNamespace();
    
    if (bytesRead != sizeof(StoredColorCapture)) {
        Serial.printf("Error: Failed to load capture %d\n", index);
        return false;
    }
    
    if (!validateCaptureData(capture)) {
        Serial.printf("Error: Invalid data in capture %d\n", index);
        return false;
    }
    
    return true;
}

bool PersistentStorage::deleteColorCapture(uint8_t index) {
    if (!initialized) return false;
    if (index >= MAX_COLOR_CAPTURES) return false;
    
    if (!openNamespace(CAPTURES_NAMESPACE, false)) {
        return false;
    }
    
    String key = generateCaptureKey(index);
    bool success = preferences.remove(key.c_str());
    
    closeNamespace();
    
    if (success) {
        Serial.printf("Deleted capture %d\n", index);
    }
    
    return success;
}

bool PersistentStorage::clearAllCaptures() {
    if (!initialized) return false;
    
    if (!openNamespace(CAPTURES_NAMESPACE, false)) {
        return false;
    }
    
    // Clear all capture keys
    for (uint8_t i = 0; i < MAX_COLOR_CAPTURES; i++) {
        String key = generateCaptureKey(i);
        preferences.remove(key.c_str());
    }
    
    // Reset counters
    totalCaptures = 0;
    currentCaptureIndex = 0;
    preferences.putUChar("totalCaptures", totalCaptures);
    preferences.putUChar("currentIndex", currentCaptureIndex);
    
    closeNamespace();
    
    Serial.println("Cleared all color captures");
    return true;
}

bool PersistentStorage::getAllCaptures(StoredColorCapture captures[], uint8_t& count) {
    if (!initialized) return false;
    
    count = 0;
    for (uint8_t i = 0; i < totalCaptures && i < MAX_COLOR_CAPTURES; i++) {
        if (loadColorCapture(i, captures[count])) {
            count++;
        }
    }
    
    Serial.printf("Loaded %d captures\n", count);
    return true;
}

bool PersistentStorage::saveCalibrationData(const StoredCalibrationData& calibData) {
    if (!initialized) return false;
    
    if (!validateCalibrationData(calibData)) {
        Serial.println("Error: Invalid calibration data");
        return false;
    }
    
    if (!openNamespace(CALIBRATION_NAMESPACE, false)) {
        Serial.println("Error: Failed to open calibration namespace");
        return false;
    }
    
    size_t bytesWritten = preferences.putBytes("calibData", &calibData, sizeof(StoredCalibrationData));
    closeNamespace();
    
    if (bytesWritten != sizeof(StoredCalibrationData)) {
        Serial.println("Error: Failed to save calibration data");
        return false;
    }
    
    // Update local copy
    calibrationData = calibData;
    
    Serial.println("Calibration data saved to flash");
    return true;
}

bool PersistentStorage::loadCalibrationData(StoredCalibrationData& calibData) {
    if (!initialized) return false;
    
    if (!openNamespace(CALIBRATION_NAMESPACE, true)) {
        Serial.println("Warning: Failed to open calibration namespace");
        return false;
    }
    
    size_t bytesRead = preferences.getBytes("calibData", &calibData, sizeof(StoredCalibrationData));
    closeNamespace();
    
    if (bytesRead != sizeof(StoredCalibrationData)) {
        Serial.println("No calibration data found in flash");
        return false;
    }
    
    if (!validateCalibrationData(calibData)) {
        Serial.println("Warning: Invalid calibration data in flash");
        return false;
    }
    
    Serial.println("Calibration data loaded from flash");
    return true;
}

bool PersistentStorage::clearCalibrationData() {
    if (!initialized) return false;
    
    if (!openNamespace(CALIBRATION_NAMESPACE, false)) {
        return false;
    }
    
    bool success = preferences.remove("calibData");
    closeNamespace();
    
    if (success) {
        // Clear local copy
        calibrationData = StoredCalibrationData();
        Serial.println("Calibration data cleared from flash");
    }
    
    return success;
}

bool PersistentStorage::hasValidCalibration() {
    return validateCalibrationData(calibrationData);
}

void PersistentStorage::printStorageInfo() {
    if (!initialized) {
        Serial.println("Storage not initialized");
        return;
    }

    Serial.println("=== Persistent Storage Info ===");
    Serial.printf("Total captures: %d/%d\n", totalCaptures, MAX_COLOR_CAPTURES);
    Serial.printf("Current index: %d\n", currentCaptureIndex);
    Serial.printf("Storage full: %s\n", isStorageFull() ? "Yes" : "No");
    Serial.printf("Has calibration: %s\n", hasValidCalibration() ? "Yes" : "No");

    if (hasValidCalibration()) {
        Serial.printf("Calibration timestamp: %u\n", calibrationData.calibrationTimestamp);
        Serial.printf("Black complete: %s\n", calibrationData.blackComplete ? "Yes" : "No");
        Serial.printf("White complete: %s\n", calibrationData.whiteComplete ? "Yes" : "No");
        Serial.printf("Blue complete: %s\n", calibrationData.blueComplete ? "Yes" : "No");
        Serial.printf("Yellow complete: %s\n", calibrationData.yellowComplete ? "Yes" : "No");
    }
}

size_t PersistentStorage::getUsedSpace() {
    if (!initialized) return 0;

    // Estimate used space based on stored data
    size_t used = 0;
    used += totalCaptures * sizeof(StoredColorCapture);
    if (hasValidCalibration()) {
        used += sizeof(StoredCalibrationData);
    }
    used += 16; // Metadata overhead

    return used;
}

size_t PersistentStorage::getFreeSpace() {
    // ESP32 NVS partition is typically 20KB, but this is a rough estimate
    constexpr size_t TOTAL_NVS_SIZE = 20480; // 20KB
    size_t used = getUsedSpace();
    return (used < TOTAL_NVS_SIZE) ? (TOTAL_NVS_SIZE - used) : 0;
}

bool PersistentStorage::getStorageStats(StorageStats& stats) {
    if (!initialized) return false;

    stats.totalCaptures = totalCaptures;
    stats.maxCaptures = MAX_COLOR_CAPTURES;
    stats.hasCalibration = hasValidCalibration();
    stats.usedBytes = getUsedSpace();
    stats.freeBytes = getFreeSpace();
    stats.oldestCaptureTimestamp = 0;
    stats.newestCaptureTimestamp = 0;

    // Find oldest and newest capture timestamps
    for (uint8_t i = 0; i < totalCaptures; i++) {
        StoredColorCapture capture;
        if (loadColorCapture(i, capture)) {
            if (stats.oldestCaptureTimestamp == 0 || capture.timestamp < stats.oldestCaptureTimestamp) {
                stats.oldestCaptureTimestamp = capture.timestamp;
            }
            if (capture.timestamp > stats.newestCaptureTimestamp) {
                stats.newestCaptureTimestamp = capture.timestamp;
            }
        }
    }

    return true;
}

// Helper functions implementation
namespace StorageHelpers {

    StoredColorCapture createCaptureFromCurrent(uint16_t x, uint16_t y, uint16_t z,
                                               uint16_t ir1, uint16_t ir2,
                                               uint8_t r, uint8_t g, uint8_t b,
                                               const String& colorName,
                                               float batteryVoltage,
                                               uint32_t searchDuration) {
        StoredColorCapture capture;
        capture.timestamp = getCurrentTimestamp(); // Use NTP-synchronized Melbourne time
        capture.x = x;
        capture.y = y;
        capture.z = z;
        capture.ir1 = ir1;
        capture.ir2 = ir2;
        capture.r = r;
        capture.g = g;
        capture.b = b;
        capture.batteryVoltage = batteryVoltage;
        capture.searchDuration = searchDuration;
        capture.isValid = true;

        // Copy color name safely
        strncpy(capture.colorName, colorName.c_str(), COLOR_NAME_MAX_LENGTH - 1);
        capture.colorName[COLOR_NAME_MAX_LENGTH - 1] = '\0';

        return capture;
    }

    StoredCalibrationData createCalibrationFromColorScience(const ColorScience::CalibrationData& calibData) {
        StoredCalibrationData stored;

        // Convert black reference
        stored.blackReference.x = (uint16_t)calibData.blackReference.raw.X;
        stored.blackReference.y = (uint16_t)calibData.blackReference.raw.Y;
        stored.blackReference.z = (uint16_t)calibData.blackReference.raw.Z;
        stored.blackReference.ir1 = (uint16_t)(calibData.blackReference.ir.IR1 * 65535);
        stored.blackReference.ir2 = (uint16_t)(calibData.blackReference.ir.IR2 * 65535);
        stored.blackReference.timestamp = calibData.blackReference.timestamp;
        stored.blackReference.quality = calibData.blackReference.quality;
        stored.blackReference.isValid = true;

        // Convert white reference
        stored.whiteReference.x = (uint16_t)calibData.whiteReference.raw.X;
        stored.whiteReference.y = (uint16_t)calibData.whiteReference.raw.Y;
        stored.whiteReference.z = (uint16_t)calibData.whiteReference.raw.Z;
        stored.whiteReference.ir1 = (uint16_t)(calibData.whiteReference.ir.IR1 * 65535);
        stored.whiteReference.ir2 = (uint16_t)(calibData.whiteReference.ir.IR2 * 65535);
        stored.whiteReference.timestamp = calibData.whiteReference.timestamp;
        stored.whiteReference.quality = calibData.whiteReference.quality;
        stored.whiteReference.isValid = true;

        // Convert blue reference if available
        if (calibData.status.blueComplete) {
            stored.blueReference.x = (uint16_t)calibData.blueReference.raw.X;
            stored.blueReference.y = (uint16_t)calibData.blueReference.raw.Y;
            stored.blueReference.z = (uint16_t)calibData.blueReference.raw.Z;
            stored.blueReference.ir1 = (uint16_t)(calibData.blueReference.ir.IR1 * 65535);
            stored.blueReference.ir2 = (uint16_t)(calibData.blueReference.ir.IR2 * 65535);
            stored.blueReference.timestamp = calibData.blueReference.timestamp;
            stored.blueReference.quality = calibData.blueReference.quality;
            stored.blueReference.isValid = true;
        }

        // Convert yellow reference if available
        if (calibData.status.yellowComplete) {
            stored.yellowReference.x = (uint16_t)calibData.yellowReference.raw.X;
            stored.yellowReference.y = (uint16_t)calibData.yellowReference.raw.Y;
            stored.yellowReference.z = (uint16_t)calibData.yellowReference.raw.Z;
            stored.yellowReference.ir1 = (uint16_t)(calibData.yellowReference.ir.IR1 * 65535);
            stored.yellowReference.ir2 = (uint16_t)(calibData.yellowReference.ir.IR2 * 65535);
            stored.yellowReference.timestamp = calibData.yellowReference.timestamp;
            stored.yellowReference.quality = calibData.yellowReference.quality;
            stored.yellowReference.isValid = true;
        }

        // Copy status flags
        stored.isCalibrated = calibData.status.is2PointCalibrated();
        stored.blackComplete = calibData.status.blackComplete;
        stored.whiteComplete = calibData.status.whiteComplete;
        stored.blueComplete = calibData.status.blueComplete;
        stored.yellowComplete = calibData.status.yellowComplete;
        stored.ledBrightness = calibData.lighting.calibrationBrightness;
        stored.calibrationTimestamp = calibData.lighting.calibrationTimestamp;

        return stored;
    }

    bool convertToColorScience(const StoredCalibrationData& stored, ColorScience::CalibrationData& calibData) {
        // Convert black reference
        calibData.blackReference.raw.X = stored.blackReference.x;
        calibData.blackReference.raw.Y = stored.blackReference.y;
        calibData.blackReference.raw.Z = stored.blackReference.z;
        calibData.blackReference.ir.IR1 = stored.blackReference.ir1 / 65535.0f;
        calibData.blackReference.ir.IR2 = stored.blackReference.ir2 / 65535.0f;
        calibData.blackReference.timestamp = stored.blackReference.timestamp;
        calibData.blackReference.quality = stored.blackReference.quality;

        // Convert white reference
        calibData.whiteReference.raw.X = stored.whiteReference.x;
        calibData.whiteReference.raw.Y = stored.whiteReference.y;
        calibData.whiteReference.raw.Z = stored.whiteReference.z;
        calibData.whiteReference.ir.IR1 = stored.whiteReference.ir1 / 65535.0f;
        calibData.whiteReference.ir.IR2 = stored.whiteReference.ir2 / 65535.0f;
        calibData.whiteReference.timestamp = stored.whiteReference.timestamp;
        calibData.whiteReference.quality = stored.whiteReference.quality;

        // Convert blue reference if valid
        if (stored.blueReference.isValid) {
            calibData.blueReference.raw.X = stored.blueReference.x;
            calibData.blueReference.raw.Y = stored.blueReference.y;
            calibData.blueReference.raw.Z = stored.blueReference.z;
            calibData.blueReference.ir.IR1 = stored.blueReference.ir1 / 65535.0f;
            calibData.blueReference.ir.IR2 = stored.blueReference.ir2 / 65535.0f;
            calibData.blueReference.timestamp = stored.blueReference.timestamp;
            calibData.blueReference.quality = stored.blueReference.quality;
        }

        // Convert yellow reference if valid
        if (stored.yellowReference.isValid) {
            calibData.yellowReference.raw.X = stored.yellowReference.x;
            calibData.yellowReference.raw.Y = stored.yellowReference.y;
            calibData.yellowReference.raw.Z = stored.yellowReference.z;
            calibData.yellowReference.ir.IR1 = stored.yellowReference.ir1 / 65535.0f;
            calibData.yellowReference.ir.IR2 = stored.yellowReference.ir2 / 65535.0f;
            calibData.yellowReference.timestamp = stored.yellowReference.timestamp;
            calibData.yellowReference.quality = stored.yellowReference.quality;
        }

        // Copy status flags (note: CalibrationStatus doesn't have isCalibrated, it has methods)
        calibData.status.blackComplete = stored.blackComplete;
        calibData.status.whiteComplete = stored.whiteComplete;
        calibData.status.blueComplete = stored.blueComplete;
        calibData.status.yellowComplete = stored.yellowComplete;
        calibData.lighting.calibrationBrightness = stored.ledBrightness;
        calibData.lighting.calibrationTimestamp = stored.calibrationTimestamp;

        return true;
    }

    bool quickSaveCurrentColor() {
        // This would be implemented to save the current color reading
        // Will be integrated with the main color capture system
        return false; // Placeholder
    }

    bool autoSaveCalibration() {
        // This would be implemented to automatically save calibration
        // Will be integrated with the calibration endpoints
        return false; // Placeholder
    }
}
