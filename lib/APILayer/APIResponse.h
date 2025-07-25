/**
 * @file APIResponse.h
 * @brief Standardized API response system for professional-grade REST API
 * 
 * This class provides a consistent, professional API response structure that
 * includes proper error handling, metadata, validation results, and structured
 * data organization. It ensures all API endpoints return responses in a
 * standardized format for reliable client integration.
 * 
 * Key Features:
 * - Consistent response structure across all endpoints
 * - Professional error handling with error codes
 * - Structured data organization with nested objects
 * - Comprehensive metadata including timing and versioning
 * - Validation result integration
 * - Automatic response formatting and serialization
 * 
 * @author Enhanced Color Calibration System
 * @version 2.0
 * @date 2025-01-24
 */

#ifndef API_RESPONSE_H
#define API_RESPONSE_H

#include "Arduino.h"
#include "ArduinoJson.h"

/**
 * @brief API response status levels
 */
enum APIResponseStatus {
    API_SUCCESS,        // Operation completed successfully
    API_WARNING,        // Operation completed with warnings
    API_ERROR,          // Operation failed with recoverable error
    API_CRITICAL_ERROR  // Operation failed with critical error
};

/**
 * @brief Error severity levels
 */
enum ErrorSeverity {
    SEVERITY_INFO,      // Informational message
    SEVERITY_WARNING,   // Warning that doesn't prevent operation
    SEVERITY_ERROR,     // Error that prevents operation
    SEVERITY_CRITICAL   // Critical error requiring immediate attention
};

/**
 * @brief Structured error information
 */
struct APIError {
    String code;            // Error code (e.g., "BRIGHTNESS_INCONSISTENT")
    String message;         // Human-readable error message
    ErrorSeverity severity; // Error severity level
    String field;           // Field that caused the error (optional)
    String details;         // Additional error details (optional)
    uint32_t timestamp;     // When error occurred
    
    APIError() : severity(SEVERITY_ERROR), timestamp(0) {}
    
    APIError(const String& errorCode, const String& errorMessage, 
             ErrorSeverity sev = SEVERITY_ERROR, const String& fieldName = "",
             const String& errorDetails = "") 
        : code(errorCode), message(errorMessage), severity(sev), 
          field(fieldName), details(errorDetails), timestamp(millis()) {}
};

/**
 * @brief Structured warning information
 */
struct APIWarning {
    String code;            // Warning code
    String message;         // Human-readable warning message
    String recommendation;  // Recommended action
    uint32_t timestamp;     // When warning occurred
    
    APIWarning() : timestamp(0) {}
    
    APIWarning(const String& warningCode, const String& warningMessage,
               const String& warningRecommendation = "")
        : code(warningCode), message(warningMessage), 
          recommendation(warningRecommendation), timestamp(millis()) {}
};

/**
 * @brief API response metadata
 */
struct APIMetadata {
    uint32_t timestamp;         // Response timestamp
    uint32_t processingTime;    // Processing time in milliseconds
    String apiVersion;          // API version
    String firmwareVersion;     // Firmware version
    String endpoint;            // API endpoint that generated response
    String requestId;           // Unique request identifier (optional)
    uint32_t memoryUsage;       // Current memory usage
    
    APIMetadata() : timestamp(0), processingTime(0), memoryUsage(0) {
        apiVersion = "2.0";
        firmwareVersion = "1.0.0";
    }
};

/**
 * @brief Professional API response builder and formatter
 */
class APIResponse {
private:
    // Response structure
    APIResponseStatus status;
    String message;
    JsonDocument* jsonDoc;
    JsonObject dataObject;
    JsonArray errorsArray;
    JsonArray warningsArray;
    JsonObject metadataObject;
    
    // Timing and metadata
    uint32_t startTime;
    APIMetadata metadata;
    
    // Configuration
    bool includeDebugInfo;
    bool includeTimingInfo;
    size_t maxResponseSize;
    
    /**
     * @brief Get status string representation
     */
    String getStatusString() const;
    
    /**
     * @brief Get severity string representation
     */
    String getSeverityString(ErrorSeverity severity) const;
    
    /**
     * @brief Finalize metadata before serialization
     */
    void finalizeMetadata();
    
public:
    /**
     * @brief Constructor
     * @param doc JsonDocument to use for response (must remain valid)
     * @param endpoint API endpoint name
     * @param requestId Optional request identifier
     */
    APIResponse(JsonDocument& doc, const String& endpoint = "", const String& requestId = "");
    
    /**
     * @brief Set response status and message
     * @param responseStatus Response status level
     * @param responseMessage Main response message
     */
    void setStatus(APIResponseStatus responseStatus, const String& responseMessage = "");
    
    /**
     * @brief Add error to response
     * @param error Structured error information
     */
    void addError(const APIError& error);
    
    /**
     * @brief Add error with basic information
     * @param code Error code
     * @param message Error message
     * @param severity Error severity
     * @param field Field that caused error (optional)
     */
    void addError(const String& code, const String& message, 
                  ErrorSeverity severity = SEVERITY_ERROR, const String& field = "");
    
    /**
     * @brief Add warning to response
     * @param warning Structured warning information
     */
    void addWarning(const APIWarning& warning);
    
    /**
     * @brief Add warning with basic information
     * @param code Warning code
     * @param message Warning message
     * @param recommendation Recommended action (optional)
     */
    void addWarning(const String& code, const String& message, const String& recommendation = "");
    
    /**
     * @brief Get data object for adding response data
     * @return JsonObject reference for adding structured data
     */
    JsonObject& getData() { return dataObject; }
    
    /**
     * @brief Add simple data field
     * @param key Field name
     * @param value Field value
     */
    template<typename T>
    void addData(const String& key, const T& value) {
        dataObject[key] = value;
    }
    
    /**
     * @brief Create nested data object
     * @param key Object name
     * @return JsonObject reference for nested data
     */
    JsonObject createDataObject(const String& key) {
        return dataObject.createNestedObject(key);
    }
    
    /**
     * @brief Create nested data array
     * @param key Array name
     * @return JsonArray reference for nested data
     */
    JsonArray createDataArray(const String& key) {
        return dataObject.createNestedArray(key);
    }
    
    /**
     * @brief Set custom metadata field
     * @param key Metadata field name
     * @param value Metadata field value
     */
    template<typename T>
    void setMetadata(const String& key, const T& value) {
        metadataObject[key] = value;
    }
    
    /**
     * @brief Enable or disable debug information
     * @param enable Whether to include debug info
     */
    void setDebugMode(bool enable) { includeDebugInfo = enable; }
    
    /**
     * @brief Enable or disable timing information
     * @param enable Whether to include timing info
     */
    void setTimingMode(bool enable) { includeTimingInfo = enable; }
    
    /**
     * @brief Check if response has errors
     * @return true if response contains errors
     */
    bool hasErrors() const { return errorsArray.size() > 0; }
    
    /**
     * @brief Check if response has warnings
     * @return true if response contains warnings
     */
    bool hasWarnings() const { return warningsArray.size() > 0; }
    
    /**
     * @brief Get current response status
     * @return Current API response status
     */
    APIResponseStatus getStatus() const { return status; }
    
    /**
     * @brief Get appropriate HTTP status code
     * @return HTTP status code based on response status
     */
    int getHTTPStatusCode() const;
    
    /**
     * @brief Serialize response to JSON string
     * @param prettyPrint Whether to format JSON with indentation
     * @return JSON string representation of response
     */
    String toJSON(bool prettyPrint = false);
    
    /**
     * @brief Get response size in bytes
     * @return Estimated response size
     */
    size_t getResponseSize() const;
    
    /**
     * @brief Validate response structure
     * @return true if response structure is valid
     */
    bool validateResponse() const;
    
    /**
     * @brief Add calibration progress information
     * @param completedSteps Number of completed calibration steps
     * @param totalSteps Total number of calibration steps
     * @param currentStep Current step name
     * @param nextStep Next step name (optional)
     */
    void addCalibrationProgress(int completedSteps, int totalSteps, 
                               const String& currentStep, const String& nextStep = "");
    
    /**
     * @brief Add environmental status information
     * @param stabilityScore Environmental stability score (0-1)
     * @param alertLevel Environmental alert level
     * @param brightnessLocked Whether brightness is locked
     * @param issues Environmental issues (optional)
     */
    void addEnvironmentalStatus(float stabilityScore, const String& alertLevel,
                               bool brightnessLocked, const String& issues = "");
    
    /**
     * @brief Add validation results
     * @param passed Whether validation passed
     * @param accuracy Validation accuracy percentage
     * @param details Validation details
     */
    void addValidationResults(bool passed, float accuracy, const String& details = "");
    
    /**
     * @brief Create error response for common scenarios
     * @param doc JsonDocument to use
     * @param errorCode Error code
     * @param errorMessage Error message
     * @param endpoint API endpoint
     * @return Formatted error response
     */
    static String createErrorResponse(JsonDocument& doc, const String& errorCode, 
                                    const String& errorMessage, const String& endpoint = "");
    
    /**
     * @brief Create success response for common scenarios
     * @param doc JsonDocument to use
     * @param message Success message
     * @param endpoint API endpoint
     * @return Formatted success response
     */
    static String createSuccessResponse(JsonDocument& doc, const String& message, 
                                      const String& endpoint = "");
};

/**
 * @brief PSRAM-aware JSON document allocator for large responses
 */
class PsramJsonAllocator {
private:
    size_t documentSize;
    
public:
    PsramJsonAllocator(size_t size = 8192) : documentSize(size) {}
    
    /**
     * @brief Create JsonDocument with PSRAM allocation if available
     * @return JsonDocument configured for optimal memory usage
     */
    JsonDocument createDocument();
    
    /**
     * @brief Get recommended document size for response type
     * @param responseType Type of response (simple, complex, data-heavy)
     * @return Recommended document size
     */
    static size_t getRecommendedSize(const String& responseType);
};

#endif // API_RESPONSE_H
