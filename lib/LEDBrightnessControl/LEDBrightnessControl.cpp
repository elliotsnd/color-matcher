/**
 * @file LEDBrightnessControl.cpp
 * @brief Implementation of LED brightness optimization system
 */

#include "LEDBrightnessControl.h"

LEDBrightnessControl::LEDBrightnessControl() : _initialized(false) {
    // Initialize default configuration
    _config.ledPin = 255;  // Invalid pin, must be set
    _config.currentBrightness = 128;
    _config.minBrightness = 20;
    _config.maxBrightness = 255;
    _config.adjustmentStep = 10;
    
    // Target signal levels (70-90% of 65535 max)
    _config.targetMin = 45000;  // ~70%
    _config.targetMax = 58000;  // ~90%
    _config.hysteresisLow = 43000;   // targetMin - 2000
    _config.hysteresisHigh = 60000;  // targetMax + 2000
    
    // Control behavior
    _config.enableAutoAdjustment = true;
    _config.adjustmentDelay = 500;
    _config.stabilizationSamples = 3;
    
    // Calibration data
    _config.whiteReferenceBrightness = 128;
    _config.whiteReferenceMaxChannel = 0;
    _config.isCalibrated = false;
    
    // Initialize statistics
    resetStatistics();
    
    _lastAdjustmentTime = 0;
    _lastMaxChannel = 0;
}

bool LEDBrightnessControl::begin(uint8_t ledPin, uint8_t initialBrightness) {
    if (ledPin > 255) return false;  // Invalid pin
    
    _config.ledPin = ledPin;
    _config.currentBrightness = constrain(initialBrightness, _config.minBrightness, _config.maxBrightness);
    
    // Configure LED pin as output
    pinMode(_config.ledPin, OUTPUT);
    applyBrightness();
    
    _initialized = true;
    return true;
}

void LEDBrightnessControl::setConfig(const LEDConfig& config) {
    _config = config;
    if (_initialized) {
        applyBrightness();
    }
}

LEDBrightnessControl::LEDConfig LEDBrightnessControl::getConfig() const {
    return _config;
}

void LEDBrightnessControl::setBrightness(uint8_t brightness) {
    _config.currentBrightness = constrain(brightness, _config.minBrightness, _config.maxBrightness);
    if (_initialized) {
        applyBrightness();
    }
}

uint8_t LEDBrightnessControl::getBrightness() const {
    return _config.currentBrightness;
}

LEDBrightnessControl::AdjustmentResult LEDBrightnessControl::optimizeBrightness(TCS3430AutoGain& colorSensor) {
    if (!_initialized || !_config.enableAutoAdjustment) {
        return NO_ADJUSTMENT_NEEDED;
    }
    
    // Check if enough time has passed since last adjustment
    unsigned long currentTime = millis();
    if (currentTime - _lastAdjustmentTime < _config.adjustmentDelay) {
        return NO_ADJUSTMENT_NEEDED;
    }
    
    // Get averaged max channel value for stability
    uint16_t maxChannel = getAveragedMaxChannel(colorSensor, _config.stabilizationSamples);
    if (maxChannel == 0) {
        return SENSOR_ERROR;
    }
    
    AdjustmentResult result = NO_ADJUSTMENT_NEEDED;
    
    // Check if within hysteresis range (prevents oscillation)
    if (isWithinHysteresis(maxChannel)) {
        result = HYSTERESIS_HOLD;
    }
    // Signal too high - decrease LED brightness
    else if (maxChannel > _config.targetMax) {
        if (_config.currentBrightness <= _config.minBrightness) {
            result = AT_MIN_BRIGHTNESS;
        } else {
            uint8_t newBrightness = _config.currentBrightness - _config.adjustmentStep;
            _config.currentBrightness = max(newBrightness, _config.minBrightness);
            applyBrightness();
            result = BRIGHTNESS_DECREASED;
            _lastAdjustmentTime = currentTime;
        }
    }
    // Signal too low - increase LED brightness
    else if (maxChannel < _config.targetMin) {
        if (_config.currentBrightness >= _config.maxBrightness) {
            result = AT_MAX_BRIGHTNESS;
        } else {
            uint8_t newBrightness = _config.currentBrightness + _config.adjustmentStep;
            _config.currentBrightness = min(newBrightness, _config.maxBrightness);
            applyBrightness();
            result = BRIGHTNESS_INCREASED;
            _lastAdjustmentTime = currentTime;
        }
    }
    
    // Update statistics
    updateStatistics(maxChannel, result);
    _lastMaxChannel = maxChannel;
    
    return result;
}

bool LEDBrightnessControl::calibrateWhiteReference(TCS3430AutoGain& colorSensor, uint16_t targetMaxChannel) {
    if (!_initialized) return false;
    
    Serial.println("=== LED BRIGHTNESS CALIBRATION ===");
    Serial.println("Place WHITE reference over sensor and ensure stable lighting");
    Serial.println("Calibrating optimal LED brightness...");
    
    // Start with current brightness
    uint8_t bestBrightness = _config.currentBrightness;
    uint16_t bestMaxChannel = 0;
    float bestError = 999999.0f;
    
    // Test different brightness levels to find optimal
    for (uint8_t brightness = _config.minBrightness; brightness <= _config.maxBrightness; brightness += 5) {
        setBrightness(brightness);
        delay(200);  // Allow LED to stabilize
        
        uint16_t maxChannel = getAveragedMaxChannel(colorSensor, 5);
        float error = abs((int32_t)maxChannel - (int32_t)targetMaxChannel);
        
        if (error < bestError) {
            bestError = error;
            bestBrightness = brightness;
            bestMaxChannel = maxChannel;
        }
        
        // Early exit if we found a very good match
        if (error < 1000) break;
    }
    
    // Set optimal brightness
    setBrightness(bestBrightness);
    
    // Update calibration data
    _config.whiteReferenceBrightness = bestBrightness;
    _config.whiteReferenceMaxChannel = bestMaxChannel;
    _config.isCalibrated = true;
    
    Serial.printf("✓ Calibration complete!\n");
    Serial.printf("  Optimal LED brightness: %d\n", bestBrightness);
    Serial.printf("  White reference max channel: %d\n", bestMaxChannel);
    Serial.printf("  Target was: %d (error: %.0f)\n", targetMaxChannel, bestError);
    
    return bestError < 3000;  // Accept if within 3000 counts
}

bool LEDBrightnessControl::isSignalOptimal(TCS3430AutoGain& colorSensor, uint16_t& maxChannel) {
    maxChannel = getMaxChannelValue(colorSensor);
    return (maxChannel >= _config.targetMin && maxChannel <= _config.targetMax);
}

void LEDBrightnessControl::enableAutoAdjustment(bool enable) {
    _config.enableAutoAdjustment = enable;
}

bool LEDBrightnessControl::isAutoAdjustmentEnabled() const {
    return _config.enableAutoAdjustment;
}

void LEDBrightnessControl::setTargetRange(uint16_t minTarget, uint16_t maxTarget, uint16_t hysteresis) {
    _config.targetMin = minTarget;
    _config.targetMax = maxTarget;
    _config.hysteresisLow = minTarget - hysteresis;
    _config.hysteresisHigh = maxTarget + hysteresis;
}

void LEDBrightnessControl::getStatistics(uint32_t& totalAdjustments, uint32_t& increasedCount, 
                                        uint32_t& decreasedCount, float& avgMaxChannel) {
    totalAdjustments = _totalAdjustments;
    increasedCount = _increasedCount;
    decreasedCount = _decreasedCount;
    avgMaxChannel = _avgMaxChannel;
}

void LEDBrightnessControl::resetStatistics() {
    _totalAdjustments = 0;
    _increasedCount = 0;
    _decreasedCount = 0;
    _avgMaxChannel = 0.0f;
    _sampleCount = 0;
}

const char* LEDBrightnessControl::adjustmentResultToString(AdjustmentResult result) {
    switch (result) {
        case NO_ADJUSTMENT_NEEDED: return "No adjustment needed";
        case BRIGHTNESS_INCREASED: return "Brightness increased";
        case BRIGHTNESS_DECREASED: return "Brightness decreased";
        case AT_MIN_BRIGHTNESS: return "At minimum brightness";
        case AT_MAX_BRIGHTNESS: return "At maximum brightness";
        case HYSTERESIS_HOLD: return "Hysteresis hold";
        case SENSOR_ERROR: return "Sensor error";
        default: return "Unknown";
    }
}

void LEDBrightnessControl::printStatus() {
    Serial.println("=== LED BRIGHTNESS STATUS ===");
    Serial.printf("Current brightness: %d/%d\n", _config.currentBrightness, _config.maxBrightness);
    Serial.printf("Target range: %d - %d counts\n", _config.targetMin, _config.targetMax);
    Serial.printf("Hysteresis range: %d - %d counts\n", _config.hysteresisLow, _config.hysteresisHigh);
    Serial.printf("Auto-adjustment: %s\n", _config.enableAutoAdjustment ? "Enabled" : "Disabled");
    Serial.printf("Calibrated: %s\n", _config.isCalibrated ? "Yes" : "No");
    
    if (_config.isCalibrated) {
        Serial.printf("White ref brightness: %d\n", _config.whiteReferenceBrightness);
        Serial.printf("White ref max channel: %d\n", _config.whiteReferenceMaxChannel);
    }
    
    Serial.printf("Total adjustments: %lu\n", _totalAdjustments);
    Serial.printf("Increased: %lu, Decreased: %lu\n", _increasedCount, _decreasedCount);
    Serial.printf("Average max channel: %.0f\n", _avgMaxChannel);
    Serial.println();
}

// Private helper functions
uint16_t LEDBrightnessControl::getMaxChannelValue(TCS3430AutoGain& colorSensor) {
    TCS3430AutoGain::RawData data = colorSensor.raw();
    return max(max(data.X, data.Y), data.Z);
}

uint16_t LEDBrightnessControl::getAveragedMaxChannel(TCS3430AutoGain& colorSensor, uint8_t samples) {
    uint32_t total = 0;
    
    for (uint8_t i = 0; i < samples; i++) {
        total += getMaxChannelValue(colorSensor);
        if (i < samples - 1) delay(50);  // Small delay between samples
    }
    
    return total / samples;
}

bool LEDBrightnessControl::isWithinHysteresis(uint16_t maxChannel) {
    // Check if we're in the hysteresis zone around the target range
    return (maxChannel > _config.hysteresisLow && maxChannel < _config.hysteresisHigh);
}

void LEDBrightnessControl::updateStatistics(uint16_t maxChannel, AdjustmentResult result) {
    if (result == BRIGHTNESS_INCREASED) {
        _totalAdjustments++;
        _increasedCount++;
    } else if (result == BRIGHTNESS_DECREASED) {
        _totalAdjustments++;
        _decreasedCount++;
    }
    
    // Update running average of max channel
    _sampleCount++;
    _avgMaxChannel = (_avgMaxChannel * (_sampleCount - 1) + maxChannel) / _sampleCount;
}

void LEDBrightnessControl::applyBrightness() {
    if (_config.ledPin <= 255) {
        analogWrite(_config.ledPin, _config.currentBrightness);
    }
}
