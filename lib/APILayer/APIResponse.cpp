/**
 * @file APIResponse.cpp
 * @brief Implementation of standardized API response system
 */

#include "APIResponse.h"

// Constructor
APIResponse::APIResponse(JsonDocument& doc, const String& endpoint, const String& requestId) 
    : jsonDoc(&doc), status(API_SUCCESS), startTime(millis()), includeDebugInfo(false), 
      includeTimingInfo(true), maxResponseSize(8192) {
    
    // Initialize JSON structure
    JsonObject root = doc.to<JsonObject>();
    dataObject = root.createNestedObject("data");
    errorsArray = root.createNestedArray("errors");
    warningsArray = root.createNestedArray("warnings");
    metadataObject = root.createNestedObject("metadata");
    
    // Set initial metadata
    metadata.timestamp = millis();
    metadata.endpoint = endpoint;
    metadata.requestId = requestId;
    metadata.memoryUsage = ESP.getFreeHeap();
    
    // Set default message
    message = "Operation completed successfully";
}

// Get status string representation
String APIResponse::getStatusString() const {
    switch (status) {
        case API_SUCCESS: return "success";
        case API_WARNING: return "warning";
        case API_ERROR: return "error";
        case API_CRITICAL_ERROR: return "critical_error";
        default: return "unknown";
    }
}

// Get severity string representation
String APIResponse::getSeverityString(ErrorSeverity severity) const {
    switch (severity) {
        case SEVERITY_INFO: return "info";
        case SEVERITY_WARNING: return "warning";
        case SEVERITY_ERROR: return "error";
        case SEVERITY_CRITICAL: return "critical";
        default: return "unknown";
    }
}

// Set response status and message
void APIResponse::setStatus(APIResponseStatus responseStatus, const String& responseMessage) {
    status = responseStatus;
    if (!responseMessage.isEmpty()) {
        message = responseMessage;
    }
}

// Add error to response
void APIResponse::addError(const APIError& error) {
    JsonObject errorObj = errorsArray.createNestedObject();
    errorObj["code"] = error.code;
    errorObj["message"] = error.message;
    errorObj["severity"] = getSeverityString(error.severity);
    errorObj["timestamp"] = error.timestamp;
    
    if (!error.field.isEmpty()) {
        errorObj["field"] = error.field;
    }
    if (!error.details.isEmpty()) {
        errorObj["details"] = error.details;
    }
    
    // Update response status based on error severity
    if (error.severity == SEVERITY_CRITICAL && status != API_CRITICAL_ERROR) {
        status = API_CRITICAL_ERROR;
    } else if (error.severity == SEVERITY_ERROR && status == API_SUCCESS) {
        status = API_ERROR;
    }
}

// Add error with basic information
void APIResponse::addError(const String& code, const String& message, 
                          ErrorSeverity severity, const String& field) {
    APIError error(code, message, severity, field);
    addError(error);
}

// Add warning to response
void APIResponse::addWarning(const APIWarning& warning) {
    JsonObject warningObj = warningsArray.createNestedObject();
    warningObj["code"] = warning.code;
    warningObj["message"] = warning.message;
    warningObj["timestamp"] = warning.timestamp;
    
    if (!warning.recommendation.isEmpty()) {
        warningObj["recommendation"] = warning.recommendation;
    }
    
    // Update response status if no errors exist
    if (status == API_SUCCESS) {
        status = API_WARNING;
    }
}

// Add warning with basic information
void APIResponse::addWarning(const String& code, const String& message, const String& recommendation) {
    APIWarning warning(code, message, recommendation);
    addWarning(warning);
}

// Get appropriate HTTP status code
int APIResponse::getHTTPStatusCode() const {
    switch (status) {
        case API_SUCCESS: return 200; // OK
        case API_WARNING: return 200; // OK with warnings
        case API_ERROR: return 400;   // Bad Request
        case API_CRITICAL_ERROR: return 500; // Internal Server Error
        default: return 500;
    }
}

// Finalize metadata before serialization
void APIResponse::finalizeMetadata() {
    metadata.processingTime = millis() - startTime;
    
    // Set metadata in JSON
    metadataObject["timestamp"] = metadata.timestamp;
    metadataObject["apiVersion"] = metadata.apiVersion;
    metadataObject["firmwareVersion"] = metadata.firmwareVersion;
    metadataObject["endpoint"] = metadata.endpoint;
    
    if (!metadata.requestId.isEmpty()) {
        metadataObject["requestId"] = metadata.requestId;
    }
    
    if (includeTimingInfo) {
        metadataObject["processingTime"] = metadata.processingTime;
    }
    
    if (includeDebugInfo) {
        metadataObject["memoryUsage"] = ESP.getFreeHeap();
        metadataObject["memoryUsageStart"] = metadata.memoryUsage;
        metadataObject["responseSize"] = getResponseSize();
    }
}

// Serialize response to JSON string
String APIResponse::toJSON(bool prettyPrint) {
    // Finalize response structure
    JsonObject root = jsonDoc->as<JsonObject>();
    root["status"] = getStatusString();
    root["message"] = message;
    
    // Only include data if it has content
    if (dataObject.size() == 0) {
        root.remove("data");
    }
    
    // Only include errors if they exist
    if (errorsArray.size() == 0) {
        root.remove("errors");
    }
    
    // Only include warnings if they exist
    if (warningsArray.size() == 0) {
        root.remove("warnings");
    }
    
    // Finalize metadata
    finalizeMetadata();
    
    // Serialize to string
    String response;
    if (prettyPrint) {
        serializeJsonPretty(*jsonDoc, response);
    } else {
        serializeJson(*jsonDoc, response);
    }
    
    return response;
}

// Get response size in bytes
size_t APIResponse::getResponseSize() const {
    return measureJson(*jsonDoc);
}

// Validate response structure
bool APIResponse::validateResponse() const {
    // Check if response size is within limits
    if (getResponseSize() > maxResponseSize) {
        return false;
    }
    
    // Check if JSON structure is valid
    JsonObject root = jsonDoc->as<JsonObject>();
    if (!root.containsKey("status") || !root.containsKey("message")) {
        return false;
    }
    
    return true;
}

// Add calibration progress information
void APIResponse::addCalibrationProgress(int completedSteps, int totalSteps, 
                                       const String& currentStep, const String& nextStep) {
    JsonObject progress = dataObject.createNestedObject("calibrationProgress");
    progress["completedSteps"] = completedSteps;
    progress["totalSteps"] = totalSteps;
    progress["progressPercent"] = (float)completedSteps / totalSteps * 100.0f;
    progress["currentStep"] = currentStep;
    
    if (!nextStep.isEmpty()) {
        progress["nextStep"] = nextStep;
    }
    
    // Add step status array
    JsonArray steps = progress.createNestedArray("steps");
    String stepNames[] = {"black", "white", "blue", "yellow"};
    for (int i = 0; i < 4; i++) {
        JsonObject step = steps.createNestedObject();
        step["name"] = stepNames[i];
        step["completed"] = (i < completedSteps);
        step["current"] = (i == completedSteps);
    }
}

// Add environmental status information
void APIResponse::addEnvironmentalStatus(float stabilityScore, const String& alertLevel,
                                       bool brightnessLocked, const String& issues) {
    JsonObject environmental = dataObject.createNestedObject("environmentalStatus");
    environmental["stabilityScore"] = stabilityScore;
    environmental["stabilityPercent"] = stabilityScore * 100.0f;
    environmental["alertLevel"] = alertLevel;
    environmental["brightnessLocked"] = brightnessLocked;
    environmental["suitable"] = (alertLevel == "none" || alertLevel == "info");
    
    if (!issues.isEmpty()) {
        environmental["issues"] = issues;
    }
    
    // Add status indicator
    String statusText = "Suitable";
    if (alertLevel == "warning") {
        statusText = "Issues Detected";
    } else if (alertLevel == "critical") {
        statusText = "Critical Issues";
    }
    environmental["statusText"] = statusText;
}

// Add validation results
void APIResponse::addValidationResults(bool passed, float accuracy, const String& details) {
    JsonObject validation = dataObject.createNestedObject("validationResults");
    validation["passed"] = passed;
    validation["accuracy"] = accuracy;
    validation["accuracyPercent"] = accuracy;
    
    if (!details.isEmpty()) {
        validation["details"] = details;
    }
    
    // Add quality assessment
    String qualityLevel = "Poor";
    if (accuracy >= 95.0f) {
        qualityLevel = "Excellent";
    } else if (accuracy >= 85.0f) {
        qualityLevel = "Good";
    } else if (accuracy >= 70.0f) {
        qualityLevel = "Fair";
    }
    validation["qualityLevel"] = qualityLevel;
}

// Create error response for common scenarios
String APIResponse::createErrorResponse(JsonDocument& doc, const String& errorCode, 
                                      const String& errorMessage, const String& endpoint) {
    APIResponse response(doc, endpoint);
    response.setStatus(API_ERROR, "Operation failed");
    response.addError(errorCode, errorMessage);
    return response.toJSON();
}

// Create success response for common scenarios
String APIResponse::createSuccessResponse(JsonDocument& doc, const String& message, 
                                        const String& endpoint) {
    APIResponse response(doc, endpoint);
    response.setStatus(API_SUCCESS, message);
    return response.toJSON();
}

// PsramJsonAllocator implementation
JsonDocument PsramJsonAllocator::createDocument() {
    // Try to use PSRAM if available, otherwise use regular heap
    #ifdef BOARD_HAS_PSRAM
    if (psramFound()) {
        return JsonDocument(documentSize);
    }
    #endif
    
    // Fallback to smaller size for regular heap
    size_t heapSize = min(documentSize, (size_t)4096);
    return JsonDocument(heapSize);
}

size_t PsramJsonAllocator::getRecommendedSize(const String& responseType) {
    if (responseType == "simple") {
        return 1024;   // Simple responses (status, basic data)
    } else if (responseType == "complex") {
        return 4096;   // Complex responses (calibration data, environmental info)
    } else if (responseType == "data-heavy") {
        return 8192;   // Data-heavy responses (validation results, reports)
    } else if (responseType == "export") {
        return 16384;  // Export responses (large data sets)
    }
    
    return 2048; // Default size
}
