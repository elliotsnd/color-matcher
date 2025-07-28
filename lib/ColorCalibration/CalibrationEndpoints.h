/**
 * @file CalibrationEndpoints.h
 * @brief REST API endpoints for color calibration system
 * @author ESP32 Color Calibration System
 * @version 1.0.0
 * @date 2024
 * 
 * This file provides REST API endpoints for managing color calibration
 * through HTTP requests, enabling web-based calibration interfaces.
 */

#ifndef CALIBRATION_ENDPOINTS_H
#define CALIBRATION_ENDPOINTS_H

#include "ColorCalibration.h"
#include <ESPAsyncWebServer.h>

// Forward declaration for global sensor reading function
extern bool readGlobalSensor(uint16_t& x, uint16_t& y, uint16_t& z, uint16_t& ir1, uint16_t& ir2);

// Forward declaration for LED brightness control
extern void setLedBrightnessForCalibration(uint8_t brightness);
extern uint8_t getCurrentLedBrightness();

/**
 * @brief REST API endpoints for color calibration
 * 
 * Provides HTTP endpoints for managing the 5-point calibration process
 * through web interfaces or REST clients.
 */
class CalibrationEndpoints {
public:
    /**
     * @brief Constructor
     * @param server ESPAsyncWebServer instance
     */
    CalibrationEndpoints(AsyncWebServer& server);
    
    /**
     * @brief Destructor
     */
    ~CalibrationEndpoints();
    
    /**
     * @brief Initialize the calibration endpoints
     * @return true if successful, false otherwise
     */
    bool initialize();
    
    /**
     * @brief Enable/disable debug mode
     * @param enabled Debug mode state
     */
    void setDebugMode(bool enabled);

private:
    AsyncWebServer& server;     ///< Reference to web server
    bool debugMode;            ///< Debug mode flag
    
    // Professional two-stage calibration handlers
    void handleCalibrateDarkOffset(AsyncWebServerRequest* request);
    void handleCalibrateBlackReference(AsyncWebServerRequest* request);

    // UNIFIED CALIBRATION ENDPOINT SYSTEM
    // Replaces 15+ redundant handlers with a single parameterized approach

    /**
     * @brief Unified calibration handler for all colors
     * @param request HTTP request object
     * @param colorName Name of the color to calibrate (e.g., "black", "white", "red")
     *
     * This single handler replaces all individual color handlers by:
     * 1. Validating the color name against the color registry
     * 2. Parsing XYZ parameters OR reading sensor automatically
     * 3. Calling addOrUpdateCalibrationPoint() with the appropriate target RGB
     * 4. Sending standardized JSON response
     */
    void handleCalibrateColor(AsyncWebServerRequest* request, const String& colorName);

    // LEGACY HANDLERS (DEPRECATED - kept for backward compatibility)
    // These will be removed in a future version - use handleCalibrateColor instead
    // Core 6-point mathematically robust calibration endpoint handlers
    void handleCalibrateBlack(AsyncWebServerRequest* request);
    void handleCalibrateWhite(AsyncWebServerRequest* request);
    void handleCalibrateRed(AsyncWebServerRequest* request);
    void handleCalibrateGreen(AsyncWebServerRequest* request);
    void handleCalibrateBlue(AsyncWebServerRequest* request);

    // Legacy calibration endpoints (optional 7th+ points)
    void handleCalibrateGrey(AsyncWebServerRequest* request);
    void handleCalibrateYellow(AsyncWebServerRequest* request);
    void handleCalibrationStatus(AsyncWebServerRequest* request);
    void handleResetCalibration(AsyncWebServerRequest* request);
    void handleCalibrationDebug(AsyncWebServerRequest* request);

    // Extended color calibration handlers (12-color support)
    void handleCalibrateVividWhite(AsyncWebServerRequest* request);

    // REMOVED HANDLERS (colors no longer supported):
    // void handleCalibrateWhite(AsyncWebServerRequest* request); - replaced with vivid-white
    // void handleCalibrateGrey(AsyncWebServerRequest* request);
    // void handleCalibrateHogBristle(AsyncWebServerRequest* request);
    // void handleCalibrateGreyPort(AsyncWebServerRequest* request);
    // void handleCalibrateDarkOffset(AsyncWebServerRequest* request);
    // void handleCalibrateBlackReference(AsyncWebServerRequest* request);

    // Auto-calibration endpoint handlers
    void handleStartAutoCalibration(AsyncWebServerRequest* request);
    void handleAutoCalibrationStatus(AsyncWebServerRequest* request);
    void handleAutoCalibrationNext(AsyncWebServerRequest* request);
    void handleAutoCalibrationRetry(AsyncWebServerRequest* request);
    void handleAutoCalibrationSkip(AsyncWebServerRequest* request);
    void handleAutoCalibrationComplete(AsyncWebServerRequest* request);

    // Enhanced status endpoints
    void handleEnhancedCalibrationStatus(AsyncWebServerRequest* request);

    // =============================================================================
    // BACKWARD COMPATIBILITY VALIDATION
    // =============================================================================

    /**
     * @brief Validate that all legacy endpoints are properly mapped to unified system
     * @return true if all legacy endpoints are functional
     */
    bool validateBackwardCompatibility();

    /**
     * @brief Get list of all legacy endpoints for compatibility testing
     * @return Vector of legacy endpoint paths
     */
    std::vector<String> getLegacyEndpoints();

    /**
     * @brief Run comprehensive system validation tests
     * @return true if all tests pass, false otherwise
     */
    bool runSystemValidation();

    // Utility functions
    String getCalibrationStatusJSON();
    String getCalibrationDebugJSON();
    String getEnhancedCalibrationStatusJSON();
    String getAutoCalibrationStatusJSON();
    bool parseCalibrationRequest(AsyncWebServerRequest* request, uint16_t& x, uint16_t& y, uint16_t& z);
    bool calibrateColorByEnum(CalibrationColor color, uint16_t x, uint16_t y, uint16_t z);
    void sendCORSResponse(AsyncWebServerRequest* request, int statusCode, const String& contentType, const String& content);

    /**
     * @brief Get valid calibration reading with automatic saturation correction
     * @param x Output X value
     * @param y Output Y value
     * @param z Output Z value
     * @return true if valid reading obtained, false if saturation uncorrectable
     */
    bool getValidCalibrationReading(uint16_t& x, uint16_t& y, uint16_t& z);
};

#endif // CALIBRATION_ENDPOINTS_H
