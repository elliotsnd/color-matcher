/**
 * @file ThresholdOptimizer.h
 * @brief Empirical threshold optimization engine for field-based calibration tuning
 * 
 * This class provides intelligent optimization of calibration thresholds and parameters
 * based on real-world usage data, environmental conditions, and validation results.
 * It enables the system to self-optimize and adapt to specific deployment environments
 * and use cases for maximum accuracy and reliability.
 * 
 * Key Features:
 * - Empirical threshold optimization based on field data
 * - Environmental condition adaptation
 * - Statistical analysis of calibration performance
 * - Automated parameter tuning with safety constraints
 * - A/B testing framework for optimization validation
 * - Machine learning-inspired optimization algorithms
 * - Performance trend analysis and prediction
 * 
 * Optimization Targets:
 * - Color validation thresholds (CIEDE2000 tolerances)
 * - Environmental stability thresholds
 * - Sensor reading quality thresholds
 * - Interpolation parameters
 * - Brightness consistency tolerances
 * - Temperature compensation factors
 * 
 * @author Enhanced Color Calibration System
 * @version 2.0
 * @date 2025-01-24
 */

#ifndef THRESHOLD_OPTIMIZER_H
#define THRESHOLD_OPTIMIZER_H

#include "Arduino.h"
#include "ValidationTestSuite.h"
#include "EnvironmentalIntegration.h"
#include "ColorScience.h"

/**
 * @brief Optimization parameter definition
 */
struct OptimizationParameter {
    String name;                // Parameter name
    float currentValue;         // Current parameter value
    float minValue;             // Minimum allowed value
    float maxValue;             // Maximum allowed value
    float stepSize;             // Optimization step size
    float bestValue;            // Best value found so far
    float bestScore;            // Best score achieved
    uint32_t optimizationCount; // Number of optimization attempts
    bool isOptimized;           // Whether parameter has been optimized
    String category;            // Parameter category (validation, environmental, etc.)
    
    OptimizationParameter() : currentValue(0), minValue(0), maxValue(1), stepSize(0.1f),
                             bestValue(0), bestScore(0), optimizationCount(0),
                             isOptimized(false), category("unknown") {}
    
    OptimizationParameter(const String& paramName, float current, float min, float max,
                         float step, const String& cat = "general")
        : name(paramName), currentValue(current), minValue(min), maxValue(max),
          stepSize(step), bestValue(current), bestScore(0), optimizationCount(0),
          isOptimized(false), category(cat) {}
};

/**
 * @brief Optimization session data
 */
struct OptimizationSession {
    uint32_t sessionId;         // Unique session identifier
    uint32_t startTime;         // Session start timestamp
    uint32_t endTime;           // Session end timestamp
    String optimizationType;    // Type of optimization performed
    uint32_t iterationsRun;     // Number of optimization iterations
    float initialScore;         // Initial performance score
    float finalScore;           // Final performance score
    float improvement;          // Performance improvement achieved
    bool successful;            // Whether optimization was successful
    String notes;               // Session notes and observations
    
    OptimizationSession() : sessionId(0), startTime(0), endTime(0),
                           iterationsRun(0), initialScore(0), finalScore(0),
                           improvement(0), successful(false) {}
};

/**
 * @brief Field data collection point
 */
struct FieldDataPoint {
    uint32_t timestamp;         // When data was collected
    float validationScore;      // Validation performance score
    float environmentalScore;   // Environmental stability score
    float conversionAccuracy;   // Color conversion accuracy
    uint16_t ambientIR1;        // Ambient IR1 level
    uint16_t ambientIR2;        // Ambient IR2 level
    float temperature;          // Temperature reading
    uint8_t ledBrightness;      // LED brightness level
    String applicationMode;     // Application mode during measurement
    bool calibrationValid;      // Whether calibration was valid
    
    FieldDataPoint() : timestamp(0), validationScore(0), environmentalScore(0),
                      conversionAccuracy(0), ambientIR1(0), ambientIR2(0),
                      temperature(25.0f), ledBrightness(0), calibrationValid(false) {}
};

/**
 * @brief Optimization algorithm configuration
 */
struct OptimizationConfig {
    String algorithm;           // Optimization algorithm ("gradient", "genetic", "bayesian")
    uint32_t maxIterations;     // Maximum optimization iterations
    float convergenceThreshold; // Convergence threshold for stopping
    float explorationRate;     // Exploration vs exploitation balance
    bool enableSafetyLimits;    // Enable safety constraint checking
    bool enableABTesting;       // Enable A/B testing validation
    uint32_t validationSamples; // Number of samples for validation
    String optimizationTarget;  // Primary optimization target
    
    OptimizationConfig() : algorithm("gradient"), maxIterations(50),
                          convergenceThreshold(0.01f), explorationRate(0.1f),
                          enableSafetyLimits(true), enableABTesting(false),
                          validationSamples(10), optimizationTarget("overall_quality") {}
};

/**
 * @brief Comprehensive threshold and parameter optimization engine
 */
class ThresholdOptimizer {
private:
    // Dependencies
    ValidationTestSuite* validationSuite;
    EnvironmentalIntegration* environmentalSystem;
    ColorScience::CalibrationData* calibrationData;
    
    // Optimization parameters
    static const int MAX_PARAMETERS = 20;
    OptimizationParameter parameters[MAX_PARAMETERS];
    int parameterCount;
    
    // Field data collection
    static const int MAX_FIELD_DATA = 100;
    FieldDataPoint fieldData[MAX_FIELD_DATA];
    int fieldDataIndex;
    int fieldDataCount;
    
    // Optimization sessions
    static const int MAX_SESSIONS = 10;
    OptimizationSession sessions[MAX_SESSIONS];
    int sessionIndex;
    int sessionCount;
    
    // Configuration
    OptimizationConfig config;
    bool optimizationEnabled;
    uint32_t lastOptimizationTime;
    
    // Statistics
    uint32_t totalOptimizations;
    uint32_t successfulOptimizations;
    float averageImprovement;
    float bestOverallScore;
    
    /**
     * @brief Initialize default optimization parameters
     */
    void initializeDefaultParameters();
    
    /**
     * @brief Calculate overall performance score
     */
    float calculatePerformanceScore();
    
    /**
     * @brief Perform gradient-based optimization
     */
    bool performGradientOptimization(OptimizationParameter& param);
    
    /**
     * @brief Perform genetic algorithm optimization
     */
    bool performGeneticOptimization();
    
    /**
     * @brief Validate optimization safety constraints
     */
    bool validateSafetyConstraints(const OptimizationParameter& param, float newValue);
    
    /**
     * @brief Apply parameter value to system
     */
    bool applyParameterValue(const OptimizationParameter& param, float value);
    
    /**
     * @brief Revert parameter to previous value
     */
    void revertParameterValue(const OptimizationParameter& param);
    
    /**
     * @brief Add field data point to collection
     */
    void addFieldDataPoint(const FieldDataPoint& dataPoint);
    
    /**
     * @brief Analyze field data trends
     */
    void analyzeFieldDataTrends();
    
    /**
     * @brief Calculate parameter correlation with performance
     */
    float calculateParameterCorrelation(const String& parameterName);
    
public:
    /**
     * @brief Constructor
     */
    ThresholdOptimizer();
    
    /**
     * @brief Initialize threshold optimizer
     * @param validationSys Validation test suite
     * @param envSystem Environmental integration system
     * @param calibData Calibration data reference
     * @return true if initialization successful
     */
    bool initialize(ValidationTestSuite* validationSys, EnvironmentalIntegration* envSystem,
                   ColorScience::CalibrationData* calibData);
    
    /**
     * @brief Add optimization parameter
     * @param parameter Parameter definition
     * @return true if parameter added successfully
     */
    bool addOptimizationParameter(const OptimizationParameter& parameter);
    
    /**
     * @brief Set optimization configuration
     * @param optimizationConfig Configuration settings
     */
    void setOptimizationConfig(const OptimizationConfig& optimizationConfig);
    
    /**
     * @brief Collect field data point
     * @param validationScore Current validation score
     * @param environmentalScore Current environmental score
     * @param conversionAccuracy Current conversion accuracy
     * @param ambientIR1 Current ambient IR1 level
     * @param ambientIR2 Current ambient IR2 level
     * @param temperature Current temperature
     * @param ledBrightness Current LED brightness
     * @param applicationMode Current application mode
     */
    void collectFieldData(float validationScore, float environmentalScore, float conversionAccuracy,
                         uint16_t ambientIR1, uint16_t ambientIR2, float temperature,
                         uint8_t ledBrightness, const String& applicationMode);
    
    /**
     * @brief Perform automatic threshold optimization
     * @param targetParameter Specific parameter to optimize (empty for all)
     * @return Optimization session results
     */
    OptimizationSession performOptimization(const String& targetParameter = "");
    
    /**
     * @brief Optimize validation thresholds based on field data
     * @return true if optimization successful
     */
    bool optimizeValidationThresholds();
    
    /**
     * @brief Optimize environmental thresholds based on field data
     * @return true if optimization successful
     */
    bool optimizeEnvironmentalThresholds();
    
    /**
     * @brief Optimize color conversion parameters
     * @return true if optimization successful
     */
    bool optimizeConversionParameters();
    
    /**
     * @brief Perform A/B testing of parameter values
     * @param parameterName Parameter to test
     * @param valueA First value to test
     * @param valueB Second value to test
     * @param testSamples Number of test samples per value
     * @return A/B test results (positive if B is better)
     */
    float performABTesting(const String& parameterName, float valueA, float valueB, int testSamples);
    
    /**
     * @brief Get optimization recommendations based on field data
     * @return Optimization recommendations
     */
    String getOptimizationRecommendations();
    
    /**
     * @brief Get parameter optimization status
     * @param parameterName Parameter name
     * @return Parameter optimization information
     */
    OptimizationParameter getParameterStatus(const String& parameterName);
    
    /**
     * @brief Get all optimization parameters
     * @param params Array to store parameters (must be pre-allocated)
     * @param maxParams Maximum number of parameters to return
     * @return Number of parameters returned
     */
    int getAllParameters(OptimizationParameter* params, int maxParams);
    
    /**
     * @brief Get field data statistics
     * @param dataPoints Array to store data points (must be pre-allocated)
     * @param maxPoints Maximum number of data points to return
     * @return Number of data points returned
     */
    int getFieldDataStatistics(FieldDataPoint* dataPoints, int maxPoints);
    
    /**
     * @brief Get optimization session history
     * @param sessionHistory Array to store sessions (must be pre-allocated)
     * @param maxSessions Maximum number of sessions to return
     * @return Number of sessions returned
     */
    int getOptimizationHistory(OptimizationSession* sessionHistory, int maxSessions);
    
    /**
     * @brief Reset optimization data and parameters
     * @param resetToDefaults Whether to reset parameters to default values
     */
    void resetOptimization(bool resetToDefaults = false);
    
    /**
     * @brief Enable or disable automatic optimization
     * @param enable Whether to enable automatic optimization
     */
    void setOptimizationEnabled(bool enable) { optimizationEnabled = enable; }
    
    /**
     * @brief Check if optimization is enabled
     */
    bool isOptimizationEnabled() const { return optimizationEnabled; }
    
    /**
     * @brief Get optimization statistics
     */
    void getOptimizationStatistics(uint32_t& totalOpts, uint32_t& successfulOpts,
                                  float& avgImprovement, float& bestScore) const;
    
    /**
     * @brief Export optimization data for analysis
     * @param includeFieldData Include field data in export
     * @param includeParameters Include parameter data in export
     * @return JSON formatted optimization data
     */
    String exportOptimizationData(bool includeFieldData = true, bool includeParameters = true);
    
    /**
     * @brief Generate optimization report
     * @return Comprehensive optimization report
     */
    String generateOptimizationReport();
    
    /**
     * @brief Predict optimal parameter values based on current conditions
     * @param ambientIR1 Current ambient IR1 level
     * @param ambientIR2 Current ambient IR2 level
     * @param temperature Current temperature
     * @param applicationMode Current application mode
     * @return Predicted optimal parameter adjustments
     */
    String predictOptimalParameters(uint16_t ambientIR1, uint16_t ambientIR2,
                                   float temperature, const String& applicationMode);
    
    /**
     * @brief Perform continuous optimization update (call periodically)
     */
    void performContinuousOptimization();
    
    /**
     * @brief Get debug information
     */
    String getDebugInfo();
};

#endif // THRESHOLD_OPTIMIZER_H
