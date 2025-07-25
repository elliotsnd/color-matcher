/**
 * @file ThresholdOptimizer.cpp
 * @brief Implementation of empirical threshold optimization engine
 */

#include "ThresholdOptimizer.h"

// Constructor
ThresholdOptimizer::ThresholdOptimizer() 
    : validationSuite(nullptr), environmentalSystem(nullptr), calibrationData(nullptr),
      parameterCount(0), fieldDataIndex(0), fieldDataCount(0),
      sessionIndex(0), sessionCount(0), optimizationEnabled(false),
      lastOptimizationTime(0), totalOptimizations(0), successfulOptimizations(0),
      averageImprovement(0.0f), bestOverallScore(0.0f) {
    
    // Initialize default configuration
    config.algorithm = "gradient";
    config.maxIterations = 50;
    config.convergenceThreshold = 0.01f;
    config.explorationRate = 0.1f;
    config.enableSafetyLimits = true;
    config.enableABTesting = false;
    config.validationSamples = 10;
    config.optimizationTarget = "overall_quality";
}

// Initialize threshold optimizer
bool ThresholdOptimizer::initialize(ValidationTestSuite* validationSys, EnvironmentalIntegration* envSystem,
                                   ColorScience::CalibrationData* calibData) {
    validationSuite = validationSys;
    environmentalSystem = envSystem;
    calibrationData = calibData;
    
    if (!validationSuite || !environmentalSystem || !calibrationData) {
        Serial.println("ERROR: Missing dependencies for threshold optimizer");
        return false;
    }
    
    Serial.println("=== Initializing Threshold Optimizer ===");
    
    // Initialize default optimization parameters
    initializeDefaultParameters();
    
    Serial.println("Threshold optimizer initialized successfully");
    Serial.println("Optimization parameters: " + String(parameterCount));
    Serial.println("Algorithm: " + config.algorithm);
    Serial.println("Max iterations: " + String(config.maxIterations));
    
    return true;
}

// Initialize default optimization parameters
void ThresholdOptimizer::initializeDefaultParameters() {
    parameterCount = 0;
    
    // Validation threshold parameters
    addOptimizationParameter(OptimizationParameter("validation_tolerance", 3.0f, 1.0f, 8.0f, 0.2f, "validation"));
    addOptimizationParameter(OptimizationParameter("excellent_threshold", 1.0f, 0.5f, 2.0f, 0.1f, "validation"));
    addOptimizationParameter(OptimizationParameter("good_threshold", 2.0f, 1.0f, 4.0f, 0.2f, "validation"));
    addOptimizationParameter(OptimizationParameter("acceptable_threshold", 3.0f, 2.0f, 6.0f, 0.2f, "validation"));
    
    // Environmental threshold parameters
    addOptimizationParameter(OptimizationParameter("ambient_change_warning", 0.15f, 0.05f, 0.30f, 0.02f, "environmental"));
    addOptimizationParameter(OptimizationParameter("ambient_change_critical", 0.30f, 0.15f, 0.50f, 0.03f, "environmental"));
    addOptimizationParameter(OptimizationParameter("temperature_change_warning", 5.0f, 2.0f, 10.0f, 0.5f, "environmental"));
    addOptimizationParameter(OptimizationParameter("stability_warning", 0.70f, 0.50f, 0.90f, 0.05f, "environmental"));
    
    // Color conversion parameters
    addOptimizationParameter(OptimizationParameter("ir_compensation_factor", 0.1f, 0.0f, 0.3f, 0.01f, "conversion"));
    addOptimizationParameter(OptimizationParameter("blue_z_ratio_min", 0.6f, 0.4f, 0.8f, 0.02f, "conversion"));
    addOptimizationParameter(OptimizationParameter("yellow_xy_ratio_min", 0.8f, 0.6f, 0.95f, 0.02f, "conversion"));
    
    Serial.println("Default optimization parameters initialized: " + String(parameterCount) + " parameters");
}

// Add optimization parameter
bool ThresholdOptimizer::addOptimizationParameter(const OptimizationParameter& parameter) {
    if (parameterCount >= MAX_PARAMETERS) {
        Serial.println("ERROR: Maximum optimization parameters reached");
        return false;
    }
    
    parameters[parameterCount] = parameter;
    parameters[parameterCount].bestValue = parameter.currentValue;
    parameterCount++;
    
    return true;
}

// Set optimization configuration
void ThresholdOptimizer::setOptimizationConfig(const OptimizationConfig& optimizationConfig) {
    config = optimizationConfig;
    Serial.println("Optimization configuration updated:");
    Serial.println("  Algorithm: " + config.algorithm);
    Serial.println("  Max iterations: " + String(config.maxIterations));
    Serial.println("  Convergence threshold: " + String(config.convergenceThreshold));
    Serial.println("  Safety limits: " + String(config.enableSafetyLimits ? "Enabled" : "Disabled"));
}

// Calculate overall performance score
float ThresholdOptimizer::calculatePerformanceScore() {
    if (!validationSuite || !environmentalSystem) {
        return 0.0f;
    }
    
    // Perform quick validation to get current performance
    ValidationResults validationResults = validationSuite->performQuickValidation(5);
    
    // Get environmental stability
    float environmentalStability = environmentalSystem->getEnvironmentalStabilityScore();
    
    // Calculate weighted performance score
    float validationScore = validationResults.overallAccuracy / 100.0f;
    float environmentalScore = environmentalStability;
    float passRateScore = validationResults.getPassRate() / 100.0f;
    
    // Weighted combination (validation is most important)
    float overallScore = (validationScore * 0.5f) + 
                        (environmentalScore * 0.2f) + 
                        (passRateScore * 0.3f);
    
    return constrain(overallScore, 0.0f, 1.0f);
}

// Collect field data point
void ThresholdOptimizer::collectFieldData(float validationScore, float environmentalScore, float conversionAccuracy,
                                         uint16_t ambientIR1, uint16_t ambientIR2, float temperature,
                                         uint8_t ledBrightness, const String& applicationMode) {
    FieldDataPoint dataPoint;
    dataPoint.timestamp = millis();
    dataPoint.validationScore = validationScore;
    dataPoint.environmentalScore = environmentalScore;
    dataPoint.conversionAccuracy = conversionAccuracy;
    dataPoint.ambientIR1 = ambientIR1;
    dataPoint.ambientIR2 = ambientIR2;
    dataPoint.temperature = temperature;
    dataPoint.ledBrightness = ledBrightness;
    dataPoint.applicationMode = applicationMode;
    dataPoint.calibrationValid = (validationScore > 0.7f && environmentalScore > 0.7f);
    
    addFieldDataPoint(dataPoint);
}

// Add field data point to collection
void ThresholdOptimizer::addFieldDataPoint(const FieldDataPoint& dataPoint) {
    fieldData[fieldDataIndex] = dataPoint;
    fieldDataIndex = (fieldDataIndex + 1) % MAX_FIELD_DATA;
    
    if (fieldDataCount < MAX_FIELD_DATA) {
        fieldDataCount++;
    }
}

// Perform gradient-based optimization
bool ThresholdOptimizer::performGradientOptimization(OptimizationParameter& param) {
    float currentScore = calculatePerformanceScore();
    float bestScore = currentScore;
    float bestValue = param.currentValue;
    
    Serial.println("Optimizing parameter: " + param.name);
    Serial.println("Current value: " + String(param.currentValue, 3));
    Serial.println("Current score: " + String(currentScore, 3));
    
    // Try increasing the parameter
    float testValue = param.currentValue + param.stepSize;
    if (testValue <= param.maxValue && validateSafetyConstraints(param, testValue)) {
        if (applyParameterValue(param, testValue)) {
            delay(1000); // Allow system to stabilize
            float testScore = calculatePerformanceScore();
            
            Serial.println("Test value (+): " + String(testValue, 3) + " -> Score: " + String(testScore, 3));
            
            if (testScore > bestScore) {
                bestScore = testScore;
                bestValue = testValue;
            }
            
            revertParameterValue(param);
        }
    }
    
    // Try decreasing the parameter
    testValue = param.currentValue - param.stepSize;
    if (testValue >= param.minValue && validateSafetyConstraints(param, testValue)) {
        if (applyParameterValue(param, testValue)) {
            delay(1000); // Allow system to stabilize
            float testScore = calculatePerformanceScore();
            
            Serial.println("Test value (-): " + String(testValue, 3) + " -> Score: " + String(testScore, 3));
            
            if (testScore > bestScore) {
                bestScore = testScore;
                bestValue = testValue;
            }
            
            revertParameterValue(param);
        }
    }
    
    // Apply best value if improvement found
    if (bestScore > currentScore + config.convergenceThreshold) {
        param.bestValue = bestValue;
        param.bestScore = bestScore;
        param.currentValue = bestValue;
        param.isOptimized = true;
        param.optimizationCount++;
        
        applyParameterValue(param, bestValue);
        
        Serial.println("Optimization successful!");
        Serial.println("Best value: " + String(bestValue, 3));
        Serial.println("Improvement: " + String((bestScore - currentScore) * 100, 1) + "%");
        
        return true;
    } else {
        Serial.println("No significant improvement found");
        return false;
    }
}

// Validate optimization safety constraints
bool ThresholdOptimizer::validateSafetyConstraints(const OptimizationParameter& param, float newValue) {
    if (!config.enableSafetyLimits) {
        return true;
    }
    
    // Check basic bounds
    if (newValue < param.minValue || newValue > param.maxValue) {
        return false;
    }
    
    // Category-specific safety checks
    if (param.category == "validation") {
        // Don't allow validation thresholds to become too strict or too loose
        if (param.name == "validation_tolerance" && (newValue < 1.0f || newValue > 8.0f)) {
            return false;
        }
    } else if (param.category == "environmental") {
        // Don't allow environmental thresholds to become too sensitive
        if (param.name.indexOf("warning") >= 0 && newValue < 0.05f) {
            return false;
        }
    }
    
    return true;
}

// Apply parameter value to system
bool ThresholdOptimizer::applyParameterValue(const OptimizationParameter& param, float value) {
    // Apply parameter to appropriate system component
    if (param.category == "validation" && validationSuite) {
        if (param.name == "validation_tolerance") {
            ValidationConfig config = validationSuite->getValidationConfig();
            config.globalTolerance = value;
            validationSuite->setValidationConfig(config);
            return true;
        } else if (param.name.indexOf("threshold") >= 0) {
            // Update CIEDE2000 quality thresholds
            float excellent, good, acceptable, poor;
            if (param.name == "excellent_threshold") {
                validationSuite->updateQualityThresholds(value, 2.0f, 3.0f, 5.0f);
            } else if (param.name == "good_threshold") {
                validationSuite->updateQualityThresholds(1.0f, value, 3.0f, 5.0f);
            } else if (param.name == "acceptable_threshold") {
                validationSuite->updateQualityThresholds(1.0f, 2.0f, value, 5.0f);
            }
            return true;
        }
    } else if (param.category == "environmental" && environmentalSystem) {
        // Apply environmental threshold changes
        // Note: This would require extending the environmental system API
        Serial.println("Environmental parameter update: " + param.name + " = " + String(value, 3));
        return true;
    } else if (param.category == "conversion" && calibrationData) {
        // Apply conversion parameter changes
        if (param.name == "ir_compensation_factor") {
            calibrationData->irCompensationFactor = value;
            return true;
        } else if (param.name == "blue_z_ratio_min") {
            calibrationData->thresholds.blueZRatioMin = value;
            return true;
        } else if (param.name == "yellow_xy_ratio_min") {
            calibrationData->thresholds.yellowXYRatioMin = value;
            return true;
        }
    }
    
    return false;
}

// Revert parameter to previous value
void ThresholdOptimizer::revertParameterValue(const OptimizationParameter& param) {
    applyParameterValue(param, param.currentValue);
}

// Perform automatic threshold optimization
OptimizationSession ThresholdOptimizer::performOptimization(const String& targetParameter) {
    OptimizationSession session;
    session.sessionId = millis();
    session.startTime = millis();
    session.optimizationType = targetParameter.isEmpty() ? "full_optimization" : "parameter_optimization";
    session.initialScore = calculatePerformanceScore();
    
    Serial.println("=== Starting Optimization Session ===");
    Serial.println("Session ID: " + String(session.sessionId));
    Serial.println("Type: " + session.optimizationType);
    Serial.println("Initial score: " + String(session.initialScore, 3));
    
    bool anyImprovement = false;
    uint32_t iterations = 0;
    
    // Optimize specific parameter or all parameters
    for (int i = 0; i < parameterCount && iterations < config.maxIterations; i++) {
        if (!targetParameter.isEmpty() && parameters[i].name != targetParameter) {
            continue;
        }
        
        Serial.println("\n--- Optimizing: " + parameters[i].name + " ---");
        
        if (config.algorithm == "gradient") {
            if (performGradientOptimization(parameters[i])) {
                anyImprovement = true;
            }
        }
        
        iterations++;
    }
    
    session.endTime = millis();
    session.iterationsRun = iterations;
    session.finalScore = calculatePerformanceScore();
    session.improvement = session.finalScore - session.initialScore;
    session.successful = anyImprovement && (session.improvement > config.convergenceThreshold);
    
    if (session.successful) {
        session.notes = "Optimization completed successfully with " + String(session.improvement * 100, 1) + "% improvement";
        successfulOptimizations++;
    } else {
        session.notes = "Optimization completed with minimal improvement";
    }
    
    // Update statistics
    totalOptimizations++;
    if (session.improvement > 0) {
        averageImprovement = ((averageImprovement * (totalOptimizations - 1)) + session.improvement) / totalOptimizations;
    }
    if (session.finalScore > bestOverallScore) {
        bestOverallScore = session.finalScore;
    }
    
    // Store session
    sessions[sessionIndex] = session;
    sessionIndex = (sessionIndex + 1) % MAX_SESSIONS;
    if (sessionCount < MAX_SESSIONS) {
        sessionCount++;
    }
    
    Serial.println("=== Optimization Session Complete ===");
    Serial.println("Final score: " + String(session.finalScore, 3));
    Serial.println("Improvement: " + String(session.improvement * 100, 1) + "%");
    Serial.println("Successful: " + String(session.successful ? "Yes" : "No"));
    
    return session;
}

// Get optimization recommendations based on field data
String ThresholdOptimizer::getOptimizationRecommendations() {
    String recommendations = "=== Optimization Recommendations ===\n";
    
    if (fieldDataCount < 5) {
        recommendations += "Insufficient field data for recommendations.\n";
        recommendations += "Collect more data points (current: " + String(fieldDataCount) + ", minimum: 5)\n";
        return recommendations;
    }
    
    // Analyze field data trends
    float avgValidationScore = 0.0f;
    float avgEnvironmentalScore = 0.0f;
    float avgConversionAccuracy = 0.0f;
    
    for (int i = 0; i < fieldDataCount; i++) {
        avgValidationScore += fieldData[i].validationScore;
        avgEnvironmentalScore += fieldData[i].environmentalScore;
        avgConversionAccuracy += fieldData[i].conversionAccuracy;
    }
    
    avgValidationScore /= fieldDataCount;
    avgEnvironmentalScore /= fieldDataCount;
    avgConversionAccuracy /= fieldDataCount;
    
    recommendations += "Field Data Analysis (" + String(fieldDataCount) + " data points):\n";
    recommendations += "  Average Validation Score: " + String(avgValidationScore, 2) + "\n";
    recommendations += "  Average Environmental Score: " + String(avgEnvironmentalScore, 2) + "\n";
    recommendations += "  Average Conversion Accuracy: " + String(avgConversionAccuracy, 2) + "\n\n";
    
    // Generate specific recommendations
    if (avgValidationScore < 0.7f) {
        recommendations += "RECOMMENDATION: Validation performance is below optimal.\n";
        recommendations += "  - Consider relaxing validation thresholds\n";
        recommendations += "  - Perform calibration optimization\n";
        recommendations += "  - Check test color quality\n\n";
    }
    
    if (avgEnvironmentalScore < 0.8f) {
        recommendations += "RECOMMENDATION: Environmental stability is suboptimal.\n";
        recommendations += "  - Adjust environmental thresholds\n";
        recommendations += "  - Improve lighting consistency\n";
        recommendations += "  - Consider environmental compensation\n\n";
    }
    
    if (avgConversionAccuracy < 0.85f) {
        recommendations += "RECOMMENDATION: Color conversion accuracy needs improvement.\n";
        recommendations += "  - Optimize interpolation parameters\n";
        recommendations += "  - Adjust IR compensation factors\n";
        recommendations += "  - Validate calibration reference points\n\n";
    }
    
    // Parameter-specific recommendations
    for (int i = 0; i < parameterCount; i++) {
        if (!parameters[i].isOptimized && parameters[i].optimizationCount == 0) {
            recommendations += "SUGGESTION: Parameter '" + parameters[i].name + "' has not been optimized.\n";
        }
    }
    
    return recommendations;
}
