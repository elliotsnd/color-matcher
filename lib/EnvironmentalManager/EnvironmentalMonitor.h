/**
 * @file EnvironmentalMonitor.h
 * @brief Comprehensive environmental monitoring for color calibration systems
 * 
 * This class provides continuous monitoring of environmental conditions that
 * affect color calibration accuracy, including ambient light, temperature,
 * vibration detection, and long-term stability tracking.
 * 
 * Key Features:
 * - Continuous ambient light monitoring
 * - Temperature stability tracking
 * - Vibration and movement detection
 * - Long-term environmental trend analysis
 * - Automatic calibration validity assessment
 * - Environmental alert system
 * 
 * @author Enhanced Color Calibration System
 * @version 2.0
 * @date 2025-01-24
 */

#ifndef ENVIRONMENTAL_MONITOR_H
#define ENVIRONMENTAL_MONITOR_H

#include "Arduino.h"
#include "CalibrationLightingManager.h"

/**
 * @brief Environmental trend data point
 */
struct EnvironmentalDataPoint {
    uint32_t timestamp;         // When measurement was taken
    uint16_t ambientIR1;        // Ambient IR1 level
    uint16_t ambientIR2;        // Ambient IR2 level
    float temperature;          // Temperature reading
    uint8_t ledBrightness;      // LED brightness at time of measurement
    float stabilityScore;       // Calculated stability score
    bool calibrationActive;     // Whether calibration was active
    
    EnvironmentalDataPoint() : timestamp(0), ambientIR1(0), ambientIR2(0), 
                              temperature(25.0f), ledBrightness(0), 
                              stabilityScore(1.0f), calibrationActive(false) {}
};

/**
 * @brief Environmental alert levels
 */
enum EnvironmentalAlertLevel {
    ALERT_NONE,         // No environmental issues
    ALERT_INFO,         // Minor environmental changes
    ALERT_WARNING,      // Moderate environmental issues
    ALERT_CRITICAL      // Severe environmental issues affecting calibration
};

/**
 * @brief Environmental alert information
 */
struct EnvironmentalAlert {
    EnvironmentalAlertLevel level;
    String message;
    String recommendation;
    uint32_t timestamp;
    bool acknowledged;
    
    EnvironmentalAlert() : level(ALERT_NONE), timestamp(0), acknowledged(false) {}
    
    EnvironmentalAlert(EnvironmentalAlertLevel lvl, const String& msg, const String& rec) 
        : level(lvl), message(msg), recommendation(rec), timestamp(millis()), acknowledged(false) {}
};

/**
 * @brief Environmental trend analysis results
 */
struct EnvironmentalTrends {
    float ambientLightTrend;        // Trend in ambient light (positive = increasing)
    float temperatureTrend;         // Trend in temperature (°C per hour)
    float stabilityTrend;           // Trend in overall stability
    uint32_t analysisTimespan;      // Timespan of analysis (ms)
    uint32_t dataPointCount;        // Number of data points analyzed
    bool trendsValid;               // Whether trend analysis is valid
    
    EnvironmentalTrends() : ambientLightTrend(0.0f), temperatureTrend(0.0f), 
                           stabilityTrend(0.0f), analysisTimespan(0), 
                           dataPointCount(0), trendsValid(false) {}
};

/**
 * @brief Comprehensive environmental monitoring system
 */
class EnvironmentalMonitor {
private:
    // Data storage
    static const int MAX_DATA_POINTS = 100;
    EnvironmentalDataPoint dataPoints[MAX_DATA_POINTS];
    int dataPointIndex;
    int dataPointCount;
    
    // Alert system
    static const int MAX_ALERTS = 20;
    EnvironmentalAlert alerts[MAX_ALERTS];
    int alertIndex;
    int alertCount;
    
    // Monitoring configuration
    bool monitoringEnabled;
    uint32_t monitoringInterval;        // Monitoring interval in ms
    uint32_t lastMonitoringTime;
    uint32_t trendAnalysisInterval;     // How often to analyze trends
    uint32_t lastTrendAnalysis;
    
    // Calibration integration
    CalibrationLightingManager* lightingManager;
    bool calibrationIntegrationEnabled;
    
    // Thresholds for alerts
    struct AlertThresholds {
        float ambientChangeWarning = 0.20f;     // 20% change warning
        float ambientChangeCritical = 0.40f;    // 40% change critical
        float temperatureChangeWarning = 5.0f;  // 5°C change warning
        float temperatureChangeCritical = 10.0f; // 10°C change critical
        float stabilityWarning = 0.70f;         // Stability below 70%
        float stabilityCritical = 0.50f;        // Stability below 50%
        uint32_t rapidChangeWindow = 60000;     // 1 minute for rapid change detection
    } alertThresholds;
    
    // Statistics
    uint32_t totalMeasurements;
    uint32_t alertsGenerated;
    uint32_t criticalAlertsGenerated;
    
    /**
     * @brief Add data point to circular buffer
     */
    void addDataPoint(const EnvironmentalDataPoint& point);
    
    /**
     * @brief Add alert to alert system
     */
    void addAlert(const EnvironmentalAlert& alert);
    
    /**
     * @brief Analyze environmental data for alerts
     */
    void analyzeForAlerts(const EnvironmentalDataPoint& currentPoint);
    
    /**
     * @brief Perform trend analysis on stored data
     */
    EnvironmentalTrends performTrendAnalysis();
    
    /**
     * @brief Calculate linear trend from data points
     */
    float calculateLinearTrend(float* values, uint32_t* timestamps, int count);
    
    /**
     * @brief Check for rapid environmental changes
     */
    void checkForRapidChanges(const EnvironmentalDataPoint& currentPoint);
    
    /**
     * @brief Get alert level name for debugging
     */
    String getAlertLevelName(EnvironmentalAlertLevel level) const;
    
public:
    /**
     * @brief Constructor
     */
    EnvironmentalMonitor();
    
    /**
     * @brief Initialize environmental monitoring
     * @param lightingMgr Pointer to lighting manager for integration
     * @param monitoringIntervalMs Monitoring interval in milliseconds
     * @return true if initialization successful
     */
    bool initialize(CalibrationLightingManager* lightingMgr = nullptr, 
                   uint32_t monitoringIntervalMs = 10000);
    
    /**
     * @brief Start environmental monitoring
     */
    void startMonitoring();
    
    /**
     * @brief Stop environmental monitoring
     */
    void stopMonitoring();
    
    /**
     * @brief Perform monitoring update (call regularly)
     * @param ir1 Current IR1 reading
     * @param ir2 Current IR2 reading
     * @param temperature Current temperature
     * @param ledBrightness Current LED brightness
     */
    void performMonitoringUpdate(uint16_t ir1, uint16_t ir2, float temperature, uint8_t ledBrightness);
    
    /**
     * @brief Get current environmental status
     * @return Current alert level
     */
    EnvironmentalAlertLevel getCurrentStatus() const;
    
    /**
     * @brief Get latest environmental trends
     */
    EnvironmentalTrends getEnvironmentalTrends();
    
    /**
     * @brief Get unacknowledged alerts
     * @param maxAlerts Maximum number of alerts to return
     * @return Number of alerts returned
     */
    int getUnacknowledgedAlerts(EnvironmentalAlert* alertBuffer, int maxAlerts);
    
    /**
     * @brief Acknowledge alert by index
     * @param alertIndex Index of alert to acknowledge
     * @return true if alert was acknowledged
     */
    bool acknowledgeAlert(int alertIndex);
    
    /**
     * @brief Acknowledge all alerts
     */
    void acknowledgeAllAlerts();
    
    /**
     * @brief Check if environmental conditions are suitable for calibration
     * @return true if conditions are suitable
     */
    bool areConditionsSuitableForCalibration() const;
    
    /**
     * @brief Get environmental stability score (0.0-1.0)
     */
    float getEnvironmentalStabilityScore() const;
    
    /**
     * @brief Get monitoring statistics
     */
    void getMonitoringStatistics(uint32_t& totalMeasurements, uint32_t& alertsGenerated, 
                                uint32_t& criticalAlerts, float& averageStability) const;
    
    /**
     * @brief Reset monitoring statistics
     */
    void resetStatistics();
    
    /**
     * @brief Update alert thresholds
     */
    void updateAlertThresholds(float ambientWarning, float ambientCritical, 
                              float tempWarning, float tempCritical,
                              float stabilityWarning, float stabilityCritical);
    
    /**
     * @brief Export environmental data as JSON
     * @param maxDataPoints Maximum number of data points to export
     */
    String exportEnvironmentalData(int maxDataPoints = 50) const;
    
    /**
     * @brief Generate environmental monitoring report
     */
    String generateMonitoringReport() const;
    
    /**
     * @brief Get debug information
     */
    String getDebugInfo() const;
    
    /**
     * @brief Check if monitoring is currently active
     */
    bool isMonitoringActive() const { return monitoringEnabled; }
    
    /**
     * @brief Get number of stored data points
     */
    int getDataPointCount() const { return dataPointCount; }
    
    /**
     * @brief Get number of active alerts
     */
    int getAlertCount() const { return alertCount; }
    
    /**
     * @brief Predict environmental stability for next period
     * @param predictionMinutes Minutes to predict ahead
     * @return Predicted stability score (0.0-1.0)
     */
    float predictEnvironmentalStability(int predictionMinutes = 30) const;
};

#endif // ENVIRONMENTAL_MONITOR_H
