/**
 * @file LEDBrightnessControl.h
 * @brief Automatic LED brightness optimization for TCS3430 color sensor
 * 
 * This library implements intelligent LED brightness control for reflective color sensing:
 * - Monitors max(X,Y,Z) channel values to prevent saturation
 * - Maintains optimal signal levels (70-90% of full scale)
 * - Includes hysteresis to prevent oscillation
 * - Supports white reference calibration
 * - Compatible with TCS3430AutoGain library
 * 
 * Target Range: 45,000-58,000 counts (70-90% of 65535 max)
 * Adjustment Step: ±10 brightness units
 * Hysteresis: ±2000 counts to prevent oscillation
 * 
 * @author LED Control System
 * @version 1.0
 * @date 2025-07-21
 */

#ifndef LED_BRIGHTNESS_CONTROL_H
#define LED_BRIGHTNESS_CONTROL_H

#include <Arduino.h>
#include "../TCS3430AutoGain/TCS3430AutoGain.h"

class LEDBrightnessControl {
public:
    // LED brightness control parameters
    struct LEDConfig {
        uint8_t ledPin;                    // PWM pin for LED control
        uint8_t currentBrightness;         // Current LED brightness (0-255)
        uint8_t minBrightness;             // Minimum allowed brightness (default: 20)
        uint8_t maxBrightness;             // Maximum allowed brightness (default: 255)
        uint8_t adjustmentStep;            // Brightness adjustment step (default: 10)
        
        // Target signal levels (counts)
        uint16_t targetMin;                // Minimum target (default: 45000, 70% of max)
        uint16_t targetMax;                // Maximum target (default: 58000, 90% of max)
        uint16_t hysteresisLow;           // Lower hysteresis threshold (targetMin - 2000)
        uint16_t hysteresisHigh;          // Upper hysteresis threshold (targetMax + 2000)
        
        // Control behavior
        bool enableAutoAdjustment;         // Enable automatic brightness control
        uint16_t adjustmentDelay;          // Delay between adjustments (ms, default: 500)
        uint8_t stabilizationSamples;      // Samples to average for stability (default: 3)
        
        // Calibration data
        uint8_t whiteReferenceBrightness;  // Optimal brightness for white reference
        uint16_t whiteReferenceMaxChannel; // Max channel value at white reference
        bool isCalibrated;                 // Whether white reference is calibrated
    };
    
    // Brightness adjustment result
    enum AdjustmentResult {
        NO_ADJUSTMENT_NEEDED,    // Signal levels are optimal
        BRIGHTNESS_INCREASED,    // LED brightness was increased
        BRIGHTNESS_DECREASED,    // LED brightness was decreased
        AT_MIN_BRIGHTNESS,       // Already at minimum, can't decrease further
        AT_MAX_BRIGHTNESS,       // Already at maximum, can't increase further
        HYSTERESIS_HOLD,         // Within hysteresis range, no change
        SENSOR_ERROR            // Sensor reading error
    };
    
    LEDBrightnessControl();
    
    /**
     * @brief Initialize LED brightness control
     * @param ledPin PWM pin connected to LED
     * @param initialBrightness Starting brightness (0-255)
     * @return true if successful
     */
    bool begin(uint8_t ledPin, uint8_t initialBrightness = 128);
    
    /**
     * @brief Set LED configuration parameters
     * @param config LED configuration structure
     */
    void setConfig(const LEDConfig& config);
    
    /**
     * @brief Get current LED configuration
     * @return Current configuration
     */
    LEDConfig getConfig() const;
    
    /**
     * @brief Manually set LED brightness
     * @param brightness Brightness level (0-255)
     */
    void setBrightness(uint8_t brightness);
    
    /**
     * @brief Get current LED brightness
     * @return Current brightness (0-255)
     */
    uint8_t getBrightness() const;
    
    /**
     * @brief Optimize LED brightness based on sensor readings
     * @param colorSensor Reference to TCS3430AutoGain sensor
     * @return AdjustmentResult indicating what action was taken
     */
    AdjustmentResult optimizeBrightness(TCS3430AutoGain& colorSensor);
    
    /**
     * @brief Calibrate LED brightness using white reference
     * @param colorSensor Reference to TCS3430AutoGain sensor
     * @param targetMaxChannel Target max channel value for white (default: 52000, 80%)
     * @return true if calibration successful
     */
    bool calibrateWhiteReference(TCS3430AutoGain& colorSensor, uint16_t targetMaxChannel = 52000);
    
    /**
     * @brief Check if current signal levels are optimal
     * @param colorSensor Reference to TCS3430AutoGain sensor
     * @param maxChannel Output parameter for max channel value
     * @return true if levels are optimal
     */
    bool isSignalOptimal(TCS3430AutoGain& colorSensor, uint16_t& maxChannel);
    
    /**
     * @brief Enable or disable automatic brightness adjustment
     * @param enable true to enable auto-adjustment
     */
    void enableAutoAdjustment(bool enable);
    
    /**
     * @brief Check if auto-adjustment is enabled
     * @return true if enabled
     */
    bool isAutoAdjustmentEnabled() const;
    
    /**
     * @brief Set target signal range
     * @param minTarget Minimum target counts (default: 45000)
     * @param maxTarget Maximum target counts (default: 58000)
     * @param hysteresis Hysteresis margin (default: 2000)
     */
    void setTargetRange(uint16_t minTarget = 45000, uint16_t maxTarget = 58000, uint16_t hysteresis = 2000);
    
    /**
     * @brief Get statistics about brightness adjustments
     * @param totalAdjustments Output: total number of adjustments made
     * @param increasedCount Output: number of brightness increases
     * @param decreasedCount Output: number of brightness decreases
     * @param avgMaxChannel Output: average max channel value
     */
    void getStatistics(uint32_t& totalAdjustments, uint32_t& increasedCount, 
                      uint32_t& decreasedCount, float& avgMaxChannel);
    
    /**
     * @brief Reset adjustment statistics
     */
    void resetStatistics();
    
    /**
     * @brief Convert adjustment result to string
     * @param result The adjustment result
     * @return String description
     */
    static const char* adjustmentResultToString(AdjustmentResult result);
    
    /**
     * @brief Print current LED status and configuration
     */
    void printStatus();
    
private:
    LEDConfig _config;
    
    // Statistics
    uint32_t _totalAdjustments;
    uint32_t _increasedCount;
    uint32_t _decreasedCount;
    float _avgMaxChannel;
    uint32_t _sampleCount;
    
    // State tracking
    unsigned long _lastAdjustmentTime;
    uint16_t _lastMaxChannel;
    bool _initialized;
    
    // Helper functions
    uint16_t getMaxChannelValue(TCS3430AutoGain& colorSensor);
    uint16_t getAveragedMaxChannel(TCS3430AutoGain& colorSensor, uint8_t samples);
    bool isWithinHysteresis(uint16_t maxChannel);
    void updateStatistics(uint16_t maxChannel, AdjustmentResult result);
    void applyBrightness();
};

#endif // LED_BRIGHTNESS_CONTROL_H
