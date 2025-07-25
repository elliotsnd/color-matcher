#ifndef PERSISTENT_STORAGE_H
#define PERSISTENT_STORAGE_H

#include "Arduino.h"
#include "Preferences.h"
#include "ArduinoJson.h"
#include "ColorScience.h"

// Storage configuration constants
constexpr uint8_t MAX_COLOR_CAPTURES = 30;
constexpr size_t COLOR_NAME_MAX_LENGTH = 64;
constexpr size_t STORAGE_NAMESPACE_MAX_LENGTH = 15;

// Storage namespaces for different data types
constexpr const char* CALIBRATION_NAMESPACE = "calibration";
constexpr const char* CAPTURES_NAMESPACE = "captures";
constexpr const char* SETTINGS_NAMESPACE = "settings";

/**
 * @brief Structure for storing a single color capture
 */
struct StoredColorCapture {
    uint32_t timestamp;           // Unix timestamp when captured
    uint16_t x, y, z;            // Raw XYZ sensor values
    uint16_t ir1, ir2;           // IR sensor values
    uint8_t r, g, b;             // Calculated RGB values (0-255)
    char colorName[COLOR_NAME_MAX_LENGTH]; // Matched color name
    float batteryVoltage;        // Battery voltage at capture time
    uint32_t searchDuration;     // Color search time in microseconds
    bool isValid;                // Flag to indicate if this slot contains valid data
    
    StoredColorCapture() : timestamp(0), x(0), y(0), z(0), ir1(0), ir2(0), 
                          r(0), g(0), b(0), batteryVoltage(0.0f), 
                          searchDuration(0), isValid(false) {
        memset(colorName, 0, COLOR_NAME_MAX_LENGTH);
    }
};

/**
 * @brief Structure for storing calibration data persistently
 */
struct StoredCalibrationData {
    // Black reference data
    struct {
        uint16_t x, y, z, ir1, ir2;
        uint32_t timestamp;
        float quality;
        bool isValid;
    } blackReference;
    
    // White reference data
    struct {
        uint16_t x, y, z, ir1, ir2;
        uint32_t timestamp;
        float quality;
        bool isValid;
    } whiteReference;
    
    // Blue reference data (optional)
    struct {
        uint16_t x, y, z, ir1, ir2;
        uint32_t timestamp;
        float quality;
        bool isValid;
    } blueReference;
    
    // Yellow reference data (optional)
    struct {
        uint16_t x, y, z, ir1, ir2;
        uint32_t timestamp;
        float quality;
        bool isValid;
    } yellowReference;
    
    // Calibration status and settings
    bool isCalibrated;
    bool blackComplete;
    bool whiteComplete;
    bool blueComplete;
    bool yellowComplete;
    uint8_t ledBrightness;
    uint32_t calibrationTimestamp;
    
    StoredCalibrationData() {
        memset(this, 0, sizeof(StoredCalibrationData));
    }
};

/**
 * @brief Main persistent storage manager class
 */
class PersistentStorage {
private:
    Preferences preferences;
    bool initialized;
    
    // Internal storage state
    uint8_t currentCaptureIndex;
    uint8_t totalCaptures;
    StoredCalibrationData calibrationData;
    
    // Helper methods
    bool openNamespace(const char* namespaceName, bool readOnly = false);
    void closeNamespace();
    String generateCaptureKey(uint8_t index);
    bool validateCaptureData(const StoredColorCapture& capture);
    bool validateCalibrationData(const StoredCalibrationData& calibData);
    
public:
    PersistentStorage();
    ~PersistentStorage();
    
    // Initialization and status
    bool begin();
    void end();
    bool isInitialized() const { return initialized; }
    
    // Color capture management
    bool saveColorCapture(const StoredColorCapture& capture);
    bool loadColorCapture(uint8_t index, StoredColorCapture& capture);
    bool deleteColorCapture(uint8_t index);
    bool clearAllCaptures();
    uint8_t getTotalCaptures() const { return totalCaptures; }
    uint8_t getMaxCaptures() const { return MAX_COLOR_CAPTURES; }
    bool isStorageFull() const { return totalCaptures >= MAX_COLOR_CAPTURES; }
    
    // Get all captures for export/display
    bool getAllCaptures(StoredColorCapture captures[], uint8_t& count);
    
    // Calibration data management
    bool saveCalibrationData(const StoredCalibrationData& calibData);
    bool loadCalibrationData(StoredCalibrationData& calibData);
    bool clearCalibrationData();
    bool hasValidCalibration();
    
    // Utility functions
    size_t getUsedSpace();
    size_t getFreeSpace();
    bool exportToJson(String& jsonOutput);
    bool importFromJson(const String& jsonInput);
    
    // Storage statistics
    struct StorageStats {
        uint8_t totalCaptures;
        uint8_t maxCaptures;
        bool hasCalibration;
        size_t usedBytes;
        size_t freeBytes;
        uint32_t oldestCaptureTimestamp;
        uint32_t newestCaptureTimestamp;
    };
    
    bool getStorageStats(StorageStats& stats);
    
    // Maintenance functions
    bool defragmentStorage();
    bool verifyDataIntegrity();
    void printStorageInfo();
};

// Global storage instance
extern PersistentStorage persistentStorage;

// Convenience functions for easy integration
namespace StorageHelpers {
    // Convert current color data to storable format
    StoredColorCapture createCaptureFromCurrent(uint16_t x, uint16_t y, uint16_t z, 
                                               uint16_t ir1, uint16_t ir2,
                                               uint8_t r, uint8_t g, uint8_t b,
                                               const String& colorName,
                                               float batteryVoltage,
                                               uint32_t searchDuration);
    
    // Convert ColorScience calibration data to storable format
    StoredCalibrationData createCalibrationFromColorScience(const ColorScience::CalibrationData& calibData);
    
    // Convert stored calibration data back to ColorScience format
    bool convertToColorScience(const StoredCalibrationData& stored, ColorScience::CalibrationData& calibData);
    
    // Quick save current color capture
    bool quickSaveCurrentColor();
    
    // Auto-save calibration after each calibration step
    bool autoSaveCalibration();
}

#endif // PERSISTENT_STORAGE_H
