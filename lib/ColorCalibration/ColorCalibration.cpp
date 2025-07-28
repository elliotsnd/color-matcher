/**
 * @file ColorCalibration.cpp
 * @brief Implementation of main color calibration interface
 * @author ESP32 Color Calibration System
 * @version 1.0.0
 * @date 2024
 * 
 * This file provides the complete implementation of the main interface
 * for the color calibration system, including drop-in replacement functions.
 */

#include "ColorCalibration.h"

// Static member definition
ColorCalibrationManager ColorCalibration::manager;

bool ColorCalibration::initialize() {
    return manager.initialize();
}

bool ColorCalibration::convertColor(uint16_t rawX, uint16_t rawY, uint16_t rawZ, uint8_t& r, uint8_t& g, uint8_t& b) {
    return manager.applyCalibrationCorrection(rawX, rawY, rawZ, r, g, b);
}

bool ColorCalibration::isCalibrated() {
    return manager.getCalibrationStatus().isComplete();
}

ColorCorrectionMatrix ColorCalibration::getColorCorrectionMatrix() {
    return manager.getColorCorrectionMatrix();
}

bool ColorCalibration::resetCalibration() {
    return manager.resetCalibration();
}

ColorCalibrationManager& ColorCalibration::getManager() {
    return manager;
}
