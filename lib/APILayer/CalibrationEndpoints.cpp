/**
 * @file CalibrationEndpoints.cpp
 * @brief Implementation of enhanced calibration API endpoints
 */

#include "CalibrationEndpoints.h"
#include "CalibrationLightingManager.h"

// Global instance
CalibrationEndpoints calibrationAPI;

// Constructor
CalibrationEndpoints::CalibrationEndpoints() 
    : environmentalSystem(nullptr), colorConverter(nullptr), calibrationData(nullptr),
      strictValidation(true), environmentalValidation(true), defaultTimeout(30000) {
}

// Initialize calibration endpoints
bool CalibrationEndpoints::initialize(EnvironmentalIntegration* envSystem, 
                                     ColorConversionEnhanced* converter,
                                     ColorScience::CalibrationData* calibData) {
    environmentalSystem = envSystem;
    colorConverter = converter;
    calibrationData = calibData;
    
    if (!environmentalSystem || !colorConverter || !calibrationData) {
        Serial.println("ERROR: Missing dependencies for calibration endpoints");
        return false;
    }
    
    Serial.println("Calibration endpoints initialized successfully");
    Serial.println("Strict validation: " + String(strictValidation ? "Enabled" : "Disabled"));
    Serial.println("Environmental validation: " + String(environmentalValidation ? "Enabled" : "Disabled"));
    
    return true;
}

// Parse calibration parameters from request
CalibrationParameters CalibrationEndpoints::parseCalibrationParameters(AsyncWebServerRequest* request) {
    CalibrationParameters params;
    
    // Parse temperature parameter
    if (request->hasParam("temperature")) {
        params.temperature = getValidatedFloatParameter(request, "temperature", 25.0f, -10.0f, 60.0f);
        params.hasTemperature = true;
    }
    
    // Parse timeout parameter
    if (request->hasParam("timeout")) {
        params.timeout = getValidatedIntParameter(request, "timeout", defaultTimeout, 5000, 300000);
        params.hasTimeout = true;
    }
    
    // Parse quality threshold parameter
    if (request->hasParam("qualityThreshold")) {
        params.qualityThreshold = getValidatedFloatParameter(request, "qualityThreshold", 0.8f, 0.0f, 1.0f);
        params.hasQualityThreshold = true;
    }
    
    // Parse validation mode parameter
    if (request->hasParam("validationMode")) {
        params.validationMode = request->getParam("validationMode")->value();
        params.hasValidationMode = true;
    }
    
    return params;
}

// Validate calibration prerequisites
bool CalibrationEndpoints::validateCalibrationPrerequisites(APIResponse& response, const String& step) {
    if (!calibrationData) {
        response.addError("CALIBRATION_DATA_MISSING", "Calibration data not available");
        return false;
    }
    
    // Check step-specific prerequisites
    if (step == "white" && !calibrationData->status.blackComplete) {
        response.addError("PREREQUISITE_MISSING", "Black reference must be calibrated before white reference",
                         SEVERITY_ERROR, "blackReference");
        return false;
    }
    
    if (step == "blue" && !calibrationData->status.is2PointCalibrated()) {
        response.addError("PREREQUISITE_MISSING", "Black and white references must be calibrated before blue reference",
                         SEVERITY_ERROR, "whiteReference");
        return false;
    }
    
    if (step == "yellow" && !calibrationData->status.blueComplete) {
        response.addError("PREREQUISITE_MISSING", "Blue reference must be calibrated before yellow reference",
                         SEVERITY_ERROR, "blueReference");
        return false;
    }
    
    return true;
}

// Perform sensor readings with validation
bool CalibrationEndpoints::performValidatedSensorReading(APIResponse& response, uint16_t& x, uint16_t& y, 
                                                         uint16_t& z, uint16_t& ir1, uint16_t& ir2, int samples) {
    // Simulate sensor reading (in real implementation, use actual sensor)
    // This would be replaced with actual TCS3430 sensor reading code
    
    uint32_t sumX = 0, sumY = 0, sumZ = 0, sumIR1 = 0, sumIR2 = 0;
    
    for (int i = 0; i < samples; i++) {
        uint16_t tempX, tempY, tempZ, tempIR1, tempIR2;
        
        // Simulate sensor reading - replace with actual sensor code:
        // colorSensor.readAll(tempX, tempY, tempZ, tempIR1, tempIR2);
        tempX = 10000 + random(-1000, 1000);
        tempY = 12000 + random(-1000, 1000);
        tempZ = 8000 + random(-1000, 1000);
        tempIR1 = 500 + random(-50, 50);
        tempIR2 = 600 + random(-50, 50);
        
        sumX += tempX;
        sumY += tempY;
        sumZ += tempZ;
        sumIR1 += tempIR1;
        sumIR2 += tempIR2;
        
        delay(50); // Allow sensor to stabilize between readings
    }
    
    // Calculate averages
    x = sumX / samples;
    y = sumY / samples;
    z = sumZ / samples;
    ir1 = sumIR1 / samples;
    ir2 = sumIR2 / samples;
    
    // Validate sensor readings
    if (x == 0 && y == 0 && z == 0) {
        response.addError("SENSOR_ERROR", "Sensor readings are all zero - check sensor connection");
        return false;
    }
    
    if (x > 60000 || y > 60000 || z > 60000) {
        response.addError("SENSOR_SATURATION", "Sensor readings are saturated - reduce light intensity");
        return false;
    }
    
    // Calculate signal quality metrics
    float totalSignal = x + y + z;
    float signalNoiseRatio = totalSignal / (ir1 + ir2 + 1); // Simplified SNR calculation
    
    if (signalNoiseRatio < 5.0f) {
        response.addWarning("LOW_SIGNAL_QUALITY", "Low signal-to-noise ratio detected",
                           "Ensure good lighting conditions and clean sensor");
    }
    
    return true;
}

// Validate color reference quality
bool CalibrationEndpoints::validateColorReferenceQuality(APIResponse& response, uint16_t x, uint16_t y, uint16_t z,
                                                        const String& colorType, float threshold) {
    float totalXYZ = x + y + z;
    
    if (colorType == "blue") {
        float zRatio = z / totalXYZ;
        if (zRatio < threshold) {
            response.addError("INSUFFICIENT_COLOR_SATURATION", 
                             "Object is not blue enough (Z ratio: " + String(zRatio, 3) + 
                             ", required: " + String(threshold, 3) + ")",
                             SEVERITY_ERROR, "colorQuality");
            response.getData()["measured"]["zRatio"] = zRatio;
            response.getData()["required"]["minZRatio"] = threshold;
            return false;
        }
        response.getData()["quality"]["zRatio"] = zRatio;
    } else if (colorType == "yellow") {
        float xyRatio = (x + y) / totalXYZ;
        if (xyRatio < threshold) {
            response.addError("INSUFFICIENT_COLOR_SATURATION",
                             "Object is not yellow enough (X+Y ratio: " + String(xyRatio, 3) + 
                             ", required: " + String(threshold, 3) + ")",
                             SEVERITY_ERROR, "colorQuality");
            response.getData()["measured"]["xyRatio"] = xyRatio;
            response.getData()["required"]["minXYRatio"] = threshold;
            return false;
        }
        response.getData()["quality"]["xyRatio"] = xyRatio;
    }
    
    return true;
}

// Add calibration quality metrics to response
void CalibrationEndpoints::addCalibrationQualityMetrics(APIResponse& response, 
                                                        const ColorScience::ReferencePoint& refPoint,
                                                        const String& referenceName) {
    JsonObject quality = response.createDataObject("qualityMetrics");
    quality["referenceName"] = referenceName;
    quality["qualityScore"] = refPoint.quality;
    quality["timestamp"] = refPoint.timestamp;
    
    // Add raw sensor values
    JsonObject rawValues = quality.createNestedObject("rawValues");
    rawValues["X"] = refPoint.raw.X;
    rawValues["Y"] = refPoint.raw.Y;
    rawValues["Z"] = refPoint.raw.Z;
    
    // Add normalized values
    JsonObject normalizedValues = quality.createNestedObject("normalizedValues");
    normalizedValues["X"] = refPoint.normalized.X;
    normalizedValues["Y"] = refPoint.normalized.Y;
    normalizedValues["Z"] = refPoint.normalized.Z;
    
    // Add IR compensation data
    JsonObject irData = quality.createNestedObject("irCompensation");
    irData["ir1Normalized"] = refPoint.ir.ir1Normalized;
    irData["ir2Normalized"] = refPoint.ir.ir2Normalized;
    irData["irRatio"] = refPoint.ir.irRatio;
}

// Enhanced black reference calibration endpoint
void CalibrationEndpoints::handleCalibrateBlackReference(AsyncWebServerRequest* request) {
    PsramJsonAllocator allocator;
    JsonDocument doc = allocator.createDocument();
    APIResponse response(doc, "/api/calibrate-black", String(millis()));
    
    Serial.println("=== Black Reference Calibration Request ===");
    
    // Parse parameters
    CalibrationParameters params = parseCalibrationParameters(request);
    
    // Validate prerequisites
    if (!validateCalibrationPrerequisites(response, "black")) {
        request->send(response.getHTTPStatusCode(), "application/json", response.toJSON());
        return;
    }
    
    // Environmental validation
    if (environmentalValidation && environmentalSystem) {
        uint16_t ir1, ir2;
        // In real implementation: colorSensor.getIR1Data(ir1); colorSensor.getIR2Data(ir2);
        ir1 = 500; ir2 = 600; // Simulated values
        
        if (!environmentalSystem->validateBlackReferenceConditions(ir1, ir2, params.temperature)) {
            response.addError("ENVIRONMENTAL_CONDITIONS_UNSUITABLE", 
                             "Environmental conditions not suitable for black reference calibration");
            response.addEnvironmentalStatus(
                environmentalSystem->getEnvironmentalStabilityScore(),
                "warning", false, "Ambient lighting or temperature issues detected");
            request->send(response.getHTTPStatusCode(), "application/json", response.toJSON());
            return;
        }
    }
    
    // Perform sensor readings
    uint16_t x, y, z, ir1, ir2;
    if (!performValidatedSensorReading(response, x, y, z, ir1, ir2, 10)) {
        request->send(response.getHTTPStatusCode(), "application/json", response.toJSON());
        return;
    }
    
    // Validate black reference quality (should be low values)
    float totalSignal = x + y + z;
    if (totalSignal > 15000) { // Threshold for "black" object
        response.addWarning("HIGH_BLACK_SIGNAL", 
                           "Black reference signal higher than expected (" + String(totalSignal) + ")",
                           "Use a darker object or reduce ambient lighting");
    }
    
    // Store calibration data
    calibrationData->blackReference.raw = {(float)x, (float)y, (float)z};
    calibrationData->blackReference.normalized = {x / 65535.0f, y / 65535.0f, z / 65535.0f};
    calibrationData->blackReference.ir = {
        ir1 / 65535.0f, ir2 / 65535.0f, 
        ir2 > 0 ? ir1 / (float)ir2 : 1.0f,
        (ir1 + ir2) / 2.0f / 65535.0f, 0.0f, 0.0f
    };
    calibrationData->blackReference.quality = totalSignal < 5000 ? 0.9f : 0.7f;
    calibrationData->blackReference.timestamp = millis();
    calibrationData->status.blackComplete = true;
    
    // Update environmental system
    if (environmentalSystem) {
        environmentalSystem->getLightingManager().advanceCalibrationPhase(SEQUENCE_BLACK_PHASE);
    }
    
    // Build response
    response.setStatus(API_SUCCESS, "Black reference calibrated successfully");
    
    // Add calibration data
    JsonObject calibrationResult = response.createDataObject("calibration");
    calibrationResult["X"] = x;
    calibrationResult["Y"] = y;
    calibrationResult["Z"] = z;
    calibrationResult["totalSignal"] = totalSignal;
    calibrationResult["samples"] = 10;
    
    // Add quality metrics
    addCalibrationQualityMetrics(response, calibrationData->blackReference, "black");
    
    // Add progress information
    response.addCalibrationProgress(1, 4, "black", "white");
    
    // Add environmental status if available
    if (environmentalSystem) {
        response.addEnvironmentalStatus(
            environmentalSystem->getEnvironmentalStabilityScore(),
            "none", false);
    }
    
    Serial.println("Black reference calibrated: X=" + String(x) + " Y=" + String(y) + " Z=" + String(z));
    
    request->send(response.getHTTPStatusCode(), "application/json", response.toJSON());
}

// Enhanced white reference calibration endpoint
void CalibrationEndpoints::handleCalibrateWhiteReference(AsyncWebServerRequest* request) {
    PsramJsonAllocator allocator;
    JsonDocument doc = allocator.createDocument();
    APIResponse response(doc, "/api/calibrate-white", String(millis()));

    Serial.println("=== White Reference Calibration Request ===");

    // Parse parameters
    CalibrationParameters params = parseCalibrationParameters(request);

    // Validate prerequisites
    if (!validateCalibrationPrerequisites(response, "white")) {
        request->send(response.getHTTPStatusCode(), "application/json", response.toJSON());
        return;
    }

    // Get LED brightness parameter
    uint8_t ledBrightness = getValidatedIntParameter(request, "brightness", 20, 1, 255);

    // Environmental validation and brightness locking
    if (environmentalValidation && environmentalSystem) {
        uint16_t ir1, ir2;
        // In real implementation: colorSensor.getIR1Data(ir1); colorSensor.getIR2Data(ir2);
        ir1 = 500; ir2 = 600; // Simulated values

        if (!environmentalSystem->validateWhiteReferenceConditions(ledBrightness, ir1, ir2, params.temperature)) {
            response.addError("ENVIRONMENTAL_CONDITIONS_UNSUITABLE",
                             "Environmental conditions not suitable or brightness locking failed");
            request->send(response.getHTTPStatusCode(), "application/json", response.toJSON());
            return;
        }

        // Verify brightness is locked
        if (!environmentalSystem->getLightingManager().isBrightnessLocked()) {
            response.addError("BRIGHTNESS_LOCK_FAILED", "Failed to lock LED brightness for calibration sequence");
            request->send(response.getHTTPStatusCode(), "application/json", response.toJSON());
            return;
        }
    }

    // Perform sensor readings
    uint16_t x, y, z, ir1, ir2;
    if (!performValidatedSensorReading(response, x, y, z, ir1, ir2, 10)) {
        request->send(response.getHTTPStatusCode(), "application/json", response.toJSON());
        return;
    }

    // Validate white reference quality (should be high values)
    float totalSignal = x + y + z;
    if (totalSignal < 30000) { // Threshold for "white" object
        response.addWarning("LOW_WHITE_SIGNAL",
                           "White reference signal lower than expected (" + String(totalSignal) + ")",
                           "Use a brighter white object or increase LED brightness");
    }

    // Check for sensor saturation
    if (x > 55000 || y > 55000 || z > 55000) {
        response.addWarning("NEAR_SATURATION",
                           "Sensor readings near saturation",
                           "Consider reducing LED brightness or using less reflective white object");
    }

    // Store calibration data
    calibrationData->whiteReference.raw = {(float)x, (float)y, (float)z};
    calibrationData->whiteReference.normalized = {x / 65535.0f, y / 65535.0f, z / 65535.0f};
    calibrationData->whiteReference.ir = {
        ir1 / 65535.0f, ir2 / 65535.0f,
        ir2 > 0 ? ir1 / (float)ir2 : 1.0f,
        (ir1 + ir2) / 2.0f / 65535.0f, 0.0f, 0.0f
    };
    calibrationData->whiteReference.quality = totalSignal > 40000 ? 0.95f : 0.8f;
    calibrationData->whiteReference.timestamp = millis();
    calibrationData->status.whiteComplete = true;

    // Store LED brightness in lighting conditions
    calibrationData->lighting.calibrationBrightness = ledBrightness;
    calibrationData->lighting.brightnessLocked = true;
    calibrationData->lighting.calibrationTimestamp = millis();

    // Update environmental system
    if (environmentalSystem) {
        environmentalSystem->getLightingManager().advanceCalibrationPhase(SEQUENCE_WHITE_PHASE);
    }

    // Initialize color converter for 2-point mode
    if (colorConverter) {
        colorConverter->reinitialize(*calibrationData);
    }

    // Build response
    response.setStatus(API_SUCCESS, "White reference calibrated successfully with brightness locked");

    // Add calibration data
    JsonObject calibrationResult = response.createDataObject("calibration");
    calibrationResult["X"] = x;
    calibrationResult["Y"] = y;
    calibrationResult["Z"] = z;
    calibrationResult["totalSignal"] = totalSignal;
    calibrationResult["ledBrightness"] = ledBrightness;
    calibrationResult["brightnessLocked"] = true;
    calibrationResult["samples"] = 10;

    // Add quality metrics
    addCalibrationQualityMetrics(response, calibrationData->whiteReference, "white");

    // Add progress information
    response.addCalibrationProgress(2, 4, "white", "blue");

    // Add environmental status
    if (environmentalSystem) {
        response.addEnvironmentalStatus(
            environmentalSystem->getEnvironmentalStabilityScore(),
            "none", true);
    }

    Serial.println("White reference calibrated: X=" + String(x) + " Y=" + String(y) + " Z=" + String(z) +
                   " (Brightness locked at " + String(ledBrightness) + ")");

    request->send(response.getHTTPStatusCode(), "application/json", response.toJSON());
}
