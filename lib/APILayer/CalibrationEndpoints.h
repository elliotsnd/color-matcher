/**
 * @file CalibrationEndpoints.h
 * @brief Enhanced calibration API endpoints with standardized responses
 * 
 * This module provides professional-grade API endpoints for the enhanced
 * 4-point color calibration system. All endpoints use the standardized
 * APIResponse system for consistent error handling, validation, and
 * structured data organization.
 * 
 * Key Features:
 * - Standardized response format across all endpoints
 * - Comprehensive error handling and validation
 * - Environmental integration for all calibration steps
 * - Progress tracking and status reporting
 * - Professional-grade parameter validation
 * - Detailed calibration metadata and quality metrics
 * 
 * @author Enhanced Color Calibration System
 * @version 2.0
 * @date 2025-01-24
 */

#ifndef CALIBRATION_ENDPOINTS_H
#define CALIBRATION_ENDPOINTS_H

#include "Arduino.h"
#include "ESPAsyncWebServer.h"
#include "APIResponse.h"
#include "EnvironmentalIntegration.h"
#include "ColorConversionEnhanced.h"
#include "ColorScience.h"

/**
 * @brief Calibration endpoint parameter validation
 */
struct CalibrationParameters {
    bool hasTemperature;
    float temperature;
    bool hasTimeout;
    uint32_t timeout;
    bool hasQualityThreshold;
    float qualityThreshold;
    bool hasValidationMode;
    String validationMode;
    
    CalibrationParameters() : hasTemperature(false), temperature(25.0f),
                             hasTimeout(false), timeout(30000),
                             hasQualityThreshold(false), qualityThreshold(0.8f),
                             hasValidationMode(false), validationMode("standard") {}
};

/**
 * @brief Enhanced calibration API endpoints manager
 */
class CalibrationEndpoints {
private:
    // Dependencies
    EnvironmentalIntegration* environmentalSystem;
    ColorConversionEnhanced* colorConverter;
    ColorScience::CalibrationData* calibrationData;
    
    // Configuration
    bool strictValidation;
    bool environmentalValidation;
    uint32_t defaultTimeout;
    
    /**
     * @brief Parse calibration parameters from request
     */
    CalibrationParameters parseCalibrationParameters(AsyncWebServerRequest* request);
    
    /**
     * @brief Validate calibration prerequisites
     */
    bool validateCalibrationPrerequisites(APIResponse& response, const String& step);
    
    /**
     * @brief Perform sensor readings with validation
     */
    bool performValidatedSensorReading(APIResponse& response, uint16_t& x, uint16_t& y, uint16_t& z,
                                      uint16_t& ir1, uint16_t& ir2, int samples = 10);
    
    /**
     * @brief Add calibration quality metrics to response
     */
    void addCalibrationQualityMetrics(APIResponse& response, const ColorScience::ReferencePoint& refPoint,
                                     const String& referenceName);
    
    /**
     * @brief Validate color reference quality
     */
    bool validateColorReferenceQuality(APIResponse& response, uint16_t x, uint16_t y, uint16_t z,
                                      const String& colorType, float threshold);
    
public:
    /**
     * @brief Constructor
     */
    CalibrationEndpoints();
    
    /**
     * @brief Initialize calibration endpoints
     * @param envSystem Environmental integration system
     * @param converter Color conversion system
     * @param calibData Calibration data reference
     * @return true if initialization successful
     */
    bool initialize(EnvironmentalIntegration* envSystem, ColorConversionEnhanced* converter,
                   ColorScience::CalibrationData* calibData);
    
    /**
     * @brief Register all calibration endpoints with web server
     * @param server AsyncWebServer instance
     */
    void registerEndpoints(AsyncWebServer& server);
    
    /**
     * @brief Enhanced black reference calibration endpoint
     * POST /api/calibrate-black
     */
    void handleCalibrateBlackReference(AsyncWebServerRequest* request);
    
    /**
     * @brief Enhanced white reference calibration endpoint
     * POST /api/calibrate-white
     */
    void handleCalibrateWhiteReference(AsyncWebServerRequest* request);
    
    /**
     * @brief Enhanced blue reference calibration endpoint
     * POST /api/calibrate-blue
     */
    void handleCalibrateBlueReference(AsyncWebServerRequest* request);
    
    /**
     * @brief Enhanced yellow reference calibration endpoint
     * POST /api/calibrate-yellow
     */
    void handleCalibrateYellowReference(AsyncWebServerRequest* request);
    
    /**
     * @brief Comprehensive calibration validation endpoint
     * POST /api/validate-calibration
     */
    void handleValidateCalibration(AsyncWebServerRequest* request);
    
    /**
     * @brief Get comprehensive calibration status
     * GET /api/calibration-status
     */
    void handleGetCalibrationStatus(AsyncWebServerRequest* request);
    
    /**
     * @brief Get detailed calibration data
     * GET /api/calibration-data
     */
    void handleGetCalibrationData(AsyncWebServerRequest* request);
    
    /**
     * @brief Reset calibration data
     * POST /api/reset-calibration
     */
    void handleResetCalibration(AsyncWebServerRequest* request);
    
    /**
     * @brief Export calibration data
     * GET /api/export-calibration
     */
    void handleExportCalibration(AsyncWebServerRequest* request);
    
    /**
     * @brief Import calibration data
     * POST /api/import-calibration
     */
    void handleImportCalibration(AsyncWebServerRequest* request);
    
    /**
     * @brief Get calibration recommendations
     * GET /api/calibration-recommendations
     */
    void handleGetCalibrationRecommendations(AsyncWebServerRequest* request);
    
    /**
     * @brief Test color conversion with current calibration
     * POST /api/test-conversion
     */
    void handleTestConversion(AsyncWebServerRequest* request);
    
    /**
     * @brief Get conversion performance statistics
     * GET /api/conversion-stats
     */
    void handleGetConversionStats(AsyncWebServerRequest* request);
    
    /**
     * @brief Benchmark conversion performance
     * POST /api/benchmark-conversion
     */
    void handleBenchmarkConversion(AsyncWebServerRequest* request);
    
    /**
     * @brief Update calibration settings
     * POST /api/calibration-settings
     */
    void handleUpdateCalibrationSettings(AsyncWebServerRequest* request);
    
    /**
     * @brief Get calibration help and documentation
     * GET /api/calibration-help
     */
    void handleGetCalibrationHelp(AsyncWebServerRequest* request);
    
    /**
     * @brief Enable or disable strict validation
     * @param enable Whether to enable strict validation
     */
    void setStrictValidation(bool enable) { strictValidation = enable; }
    
    /**
     * @brief Enable or disable environmental validation
     * @param enable Whether to enable environmental validation
     */
    void setEnvironmentalValidation(bool enable) { environmentalValidation = enable; }
    
    /**
     * @brief Set default timeout for calibration operations
     * @param timeoutMs Timeout in milliseconds
     */
    void setDefaultTimeout(uint32_t timeoutMs) { defaultTimeout = timeoutMs; }
    
    /**
     * @brief Get endpoint statistics
     */
    void getEndpointStatistics(uint32_t& totalRequests, uint32_t& successfulRequests,
                              uint32_t& failedRequests, float& successRate) const;
    
    /**
     * @brief Reset endpoint statistics
     */
    void resetStatistics();
};

/**
 * @brief Global calibration endpoints instance
 */
extern CalibrationEndpoints calibrationAPI;

/**
 * @brief Utility functions for calibration endpoints
 */

/**
 * @brief Validate request parameters
 * @param request HTTP request
 * @param requiredParams Array of required parameter names
 * @param paramCount Number of required parameters
 * @return true if all required parameters are present
 */
bool validateRequestParameters(AsyncWebServerRequest* request, const String* requiredParams, int paramCount);

/**
 * @brief Get request parameter as float with validation
 * @param request HTTP request
 * @param paramName Parameter name
 * @param defaultValue Default value if parameter not found
 * @param minValue Minimum allowed value
 * @param maxValue Maximum allowed value
 * @return Parameter value or default if invalid
 */
float getValidatedFloatParameter(AsyncWebServerRequest* request, const String& paramName,
                                float defaultValue, float minValue, float maxValue);

/**
 * @brief Get request parameter as integer with validation
 * @param request HTTP request
 * @param paramName Parameter name
 * @param defaultValue Default value if parameter not found
 * @param minValue Minimum allowed value
 * @param maxValue Maximum allowed value
 * @return Parameter value or default if invalid
 */
int getValidatedIntParameter(AsyncWebServerRequest* request, const String& paramName,
                            int defaultValue, int minValue, int maxValue);

/**
 * @brief Create standardized error response for missing dependencies
 * @param doc JsonDocument for response
 * @param missingComponent Name of missing component
 * @return Formatted error response
 */
String createDependencyErrorResponse(JsonDocument& doc, const String& missingComponent);

/**
 * @brief Create standardized error response for invalid parameters
 * @param doc JsonDocument for response
 * @param parameterName Name of invalid parameter
 * @param expectedFormat Expected parameter format
 * @return Formatted error response
 */
String createParameterErrorResponse(JsonDocument& doc, const String& parameterName, 
                                   const String& expectedFormat);

/**
 * @brief Log API request for debugging and monitoring
 * @param endpoint Endpoint name
 * @param method HTTP method
 * @param clientIP Client IP address
 * @param userAgent User agent string
 * @param processingTime Processing time in milliseconds
 * @param responseStatus Response status
 */
void logAPIRequest(const String& endpoint, const String& method, const String& clientIP,
                  const String& userAgent, uint32_t processingTime, APIResponseStatus responseStatus);

#endif // CALIBRATION_ENDPOINTS_H
