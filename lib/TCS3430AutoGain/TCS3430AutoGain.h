/**
 * @file TCS3430AutoGain.h
 * @brief Arduino library for the TCS3430 XYZ Color Sensor with automatic gain and integration time adjustment.
 *
 * This library mimics the functionality of the TCS34725AutoGain library, providing compatibility and advanced features
 * for highly accurate color matching. It supports automatic adjustment of gain and integration time to achieve optimal
 * sensor readings, calculation of lux and color temperature with glass attenuation compensation, and fine-grained control
 * over sensor modes, interrupts, and configurations.
 *
 * Key features:
 * - Automatic gain and integration time adjustment to meet a minimum Y-channel count for reliable measurements.
 * - Support for blocking reads and sensor mode control (Sleep, Idle, ALS, WaitALS).
 * - Calculation of chromaticity (x, y), approximate lux, and color temperature (CCT) with optional glass attenuation.
 * - Interrupt threshold and persistence settings.
 * - Raw data access for X, Y, Z, IR1, IR2 channels.
 *
 * The library is designed for production use, with robust error handling and compatibility with ESP32/Arduino platforms.
 * It assumes I2C communication and does not install additional dependencies.
 *
 * @author Inspired by kevinstadler/TCS34725AutoGain, adapted for TCS3430.
 * @version 1.0
 * @date 2025-07-21
 *
 * MIT License
 */

#ifndef TCS3430_AUTO_GAIN_H
#define TCS3430_AUTO_GAIN_H

#include <Arduino.h>
#include <Wire.h>
#include "../ColorScience/ColorScience.h"

#define TCS3430_ADDRESS         0x39

// Registers (command-included addresses)
#define TCS3430_ENABLE          0x80
#define TCS3430_ATIME           0x81
#define TCS3430_WTIME           0x83
#define TCS3430_AILTL           0x84
#define TCS3430_AILTH           0x85
#define TCS3430_AIHTL           0x86
#define TCS3430_AIHTH           0x87
#define TCS3430_PERS            0x8C
#define TCS3430_CFG0            0x8D
#define TCS3430_CFG1            0x90
#define TCS3430_REVID           0x91
#define TCS3430_ID              0x92
#define TCS3430_STATUS          0x93
#define TCS3430_CH0DATAL        0x94  // Z low
#define TCS3430_CH0DATAH        0x95  // Z high
#define TCS3430_CH1DATAL        0x96  // Y low
#define TCS3430_CH1DATAH        0x97  // Y high
#define TCS3430_CH2DATAL        0x98  // IR1 low
#define TCS3430_CH2DATAH        0x99  // IR1 high
#define TCS3430_CH3DATAL        0x9A  // X low (or IR2 if AMUX=1)
#define TCS3430_CH3DATAH        0x9B  // X high
#define TCS3430_CH4DATAL        0x9C  // IR2 low
#define TCS3430_CH4DATAH        0x9D  // IR2 high

// Magic numbers for calculations (adapted from ams DN40 for similar sensors, McCamy for CCT)
#define TCS3430_STEP_MS         2.78f  // Integration step in ms
#define TCS3430_LONG_WAIT_MUL   12.0f  // Wait long multiplier

class TCS3430AutoGain {
public:
    enum Gain { X01 = 0, X04 = 1, X16 = 2, X64 = 3 };
    enum Mode { UNDEFINED = -1, SLEEP, IDLE, ALS, WAIT_ALS };

    struct RawData {
        uint16_t X;
        uint16_t Y;
        uint16_t Z;
        uint16_t IR1;
        uint16_t IR2;
    };

    TCS3430AutoGain() : _calibData(ColorScience::createDefaultCalibration()) {
    }

    /**
     * @brief Initializes the sensor.
     * @param w The I2C Wire object (default: Wire).
     * @param addr The I2C address (default: 0x39).
     * @return true if initialization succeeds, false otherwise.
     */
    bool begin(TwoWire &w = Wire, uint8_t addr = TCS3430_ADDRESS);

    /**
     * @brief Powers the sensor on or off.
     * @param b true to power on, false to power off.
     */
    void power(bool b);

    /**
     * @brief Sets or gets the sensor mode.
     * @param m The mode to set (Undefined to get current mode).
     * @return The current mode.
     */
    Mode mode(Mode m = UNDEFINED);

    /**
     * @brief Sets or gets the integration time in milliseconds.
     * @param ms The time to set (-1 to get current).
     * @return The current integration time in ms.
     */
    float integrationTime(float ms = -1.0f);

    /**
     * @brief Sets or gets the integration cycles (1-256).
     * @param cycles The cycles to set (-1 to get current).
     * @return The current integration cycles.
     */
    int16_t integrationCycles(int16_t cycles = -1);

    /**
     * @brief Sets or gets the gain.
     * @param g The gain to set ((Gain)-1 to get current).
     * @return The current gain multiplier (1, 4, 16, 64).
     */
    float gain(Gain g = (Gain)-1);

    /**
     * @brief Performs automatic gain and integration time adjustment.
     * @param minYCount Minimum Y-channel count for reliable measurement (default: 100).
     * @param initGain Initial gain to start with (default: X01).
     * @return true if successful, false if unable to achieve min count.
     *
     * After success, a valid measurement is available via raw().
     */
    bool autoGain(uint16_t minYCount = 100, Gain initGain = X01);

    /**
     * @brief Performs a single blocking readout.
     * @return true if successful.
     */
    bool singleRead();

    /**
     * @brief Gets the raw channel data (assumes AMUX=0 for X on CH3).
     * @return RawData structure with X, Y, Z, IR1, IR2.
     */
    RawData raw();

    /**
     * @brief Checks if a measurement is available (always false for non-blocking; use singleRead() for blocking).
     * @param timeoutMs Timeout in ms (unused).
     * @return false (non-blocking not supported due to lack of AVALID bit).
     */
    static bool available(float /*timeoutMs*/ = 0.0f) {
      return false;
    }  // Placeholder for compatibility

    /**
     * @brief Checks if an interrupt is active.
     * @return true if ALS interrupt is set.
     */
    bool interrupt();

    /**
     * @brief Gets the current persistence setting.
     * @return Persistence value (0-15).
     */
    uint8_t persistence();

    /**
     * @brief Sets the persistence (consecutive out-of-range cycles for interrupt).
     * @param p Persistence value (0-15).
     */
    void persistence(uint8_t p);

    /**
     * @brief Gets the low interrupt threshold.
     * @return Low threshold.
     */
    uint16_t lowInterruptThreshold();

    /**
     * @brief Gets the high interrupt threshold.
     * @return High threshold.
     */
    uint16_t highInterruptThreshold();

    /**
     * @brief Sets the interrupt thresholds.
     * @param low Low threshold.
     * @param high High threshold.
     */
    void interruptThresholds(uint16_t low, uint16_t high);

    /**
     * @brief Sets or gets the wait time in ms.
     * @param ms Time to set (-1 to get).
     * @param enterWaitALS If true, enters WaitALS mode (default: false).
     * @return Current wait time in ms.
     */
    float wait(float ms = -1.0f, bool enterWaitALS = false);

    /**
     * @brief Enables or disables color temperature and lux calculations.
     * @param b true to enable.
     */
    void enableColorTempAndLuxCalculation(bool b) { _calcEnabled = b; }

    /**
     * @brief Sets the glass attenuation factor (transmissivity, default: 1.0 no glass).
     * @param ga Attenuation factor (e.g., 0.5 for 50% transmissivity).
     */
    void glassAttenuation(float ga) { _ga = ga; }

    /**
     * @brief Calculates approximate lux (based on Y channel, adjusted for GA).
     * @return Lux value.
     */
    float lux();

    /**
     * @brief Calculates color temperature in Kelvin using McCamy's formula.
     * @return CCT value.
     */
    float colorTemp();

    /**
     * @brief Calculates chromaticity x.
     * @return x value.
     */
    float chromaticityX();

    /**
     * @brief Calculates chromaticity y.
     * @return y value.
     */
    float chromaticityY();

    /**
     * @brief Sets the ALS multiplexer for CH3 (X or IR2).
     * @param ir2 true for IR2 on CH3, false for X (default: false).
     */
    void setAMux(bool ir2);

    /**
     * @brief Gets the device status.
     * @return Status register value.
     */
    uint8_t getDeviceStatus() { return read8(TCS3430_STATUS); }

    // ========================================
    // ADVANCED COLOR SCIENCE METHODS
    // ========================================

    /**
     * @brief Convert raw sensor data to accurate RGB using proper color science
     * @param useAdvancedColorScience If true, uses matrix conversion with IR compensation
     * @return RGB color with both float and 8-bit values
     */
    ColorScience::RGBColor getRGBColor(bool useAdvancedColorScience = true);

    /**
     * @brief Get XYZ color with IR and ambient compensation
     * @return Compensated XYZ color
     */
    ColorScience::XYZColor getXYZColor();

    /**
     * @brief Set calibration data for advanced color conversion
     * @param calibData Calibration data including white/black references
     */
    void setCalibrationData(const ColorScience::CalibrationData& calibData);

    /**
     * @brief Get current calibration data
     * @return Current calibration data
     */
    ColorScience::CalibrationData getCalibrationData() const;

    /**
     * @brief Perform white reference calibration
     * @param numSamples Number of samples to average (default: 10)
     * @return true if successful
     */
    bool calibrateWhiteReference(int numSamples = 10);

    /**
     * @brief Perform black reference calibration (cover sensor)
     * @param numSamples Number of samples to average (default: 10)
     * @return true if successful
     */
    bool calibrateBlackReference(int numSamples = 10);

    /**
     * @brief Enable/disable advanced color science features
     * @param enableIRCompensation Enable adaptive IR compensation
     * @param enableAmbientCompensation Enable black reference subtraction
     * @param irCompensationFactor IR compensation strength (0.0-1.0)
     */
    void configureColorScience(bool enableIRCompensation = true,
                              bool enableAmbientCompensation = true,
                              float irCompensationFactor = 0.1f);

    /**
     * @brief Set color space for conversion
     * @param useAdobeRGB If true, uses Adobe RGB; if false, uses sRGB
     */
    void setColorSpace(bool useAdobeRGB = false);

    /**
     * @brief Set custom conversion matrix
     * @param matrix Custom 3x3 conversion matrix (XYZ to RGB)
     */
    void setCustomMatrix(const float matrix[9]);

    /**
     * @brief Configure LED-specific IR compensation
     * @param baseCompensation Base IR compensation level (0.0-0.3)
     * @param brightnessResponse How compensation changes with LED brightness (0.0-0.1)
     * @param adaptToBrightness Enable adaptive compensation based on LED level
     */
    void configureLEDIRCompensation(float baseCompensation = 0.08f,
                                   float brightnessResponse = 0.02f,
                                   bool adaptToBrightness = true);

    /**
     * @brief Set channel-specific IR leakage compensation
     * @param xLeakage X channel IR leakage factor (typically 0.02-0.05)
     * @param yLeakage Y channel IR leakage factor (typically 0.01-0.03)
     * @param zLeakage Z channel IR leakage factor (typically 0.05-0.15)
     */
    void setChannelIRLeakage(float xLeakage = 0.03f,
                            float yLeakage = 0.015f,
                            float zLeakage = 0.08f);

    /**
     * @brief Calibrate IR compensation using LED at different brightness levels
     * @param numBrightnessLevels Number of LED brightness levels to test (3-10)
     * @param samplesPerLevel Number of samples per brightness level (5-20)
     * @return true if calibration successful
     */
    bool calibrateLEDIRResponse(int numBrightnessLevels = 5, int samplesPerLevel = 10);

    // ========================================
    // COMPATIBILITY LAYER FOR OLD TCS3430 API
    // ========================================

    // Old gain enum compatibility - using different name to avoid conflicts
    enum class OldGain { GAIN_0_25X = 0, GAIN_1X, GAIN_4X, GAIN_16X, GAIN_64X, GAIN_128X };

    /**
     * @brief Compatibility wrapper for old setGain(OldGain)
     */
    void setGain(OldGain g) {
        switch (g) {
            case OldGain::GAIN_1X: gain(X01); break;
            case OldGain::GAIN_4X: gain(X04); break;
            case OldGain::GAIN_16X: gain(X16); break;
            case OldGain::GAIN_64X: gain(X64); break;
            default: gain(X16); break; // Default to 16x for unsupported gains
        }
    }

    /**
     * @brief Compatibility wrapper for old getGain()
     */
    OldGain getGain() {
      float const G = gain();
      if (G <= 1.5f) {
        return OldGain::GAIN_1X;
      }
      if (G <= 6.0f) {
        return OldGain::GAIN_4X;
      }
      if (G <= 24.0f) {
        return OldGain::GAIN_16X;
      }
        return OldGain::GAIN_64X;
    }

    /**
     * @brief Compatibility wrapper for setIntegrationTime(float)
     */
    void setIntegrationTime(float ms) { integrationTime(ms); }

    /**
     * @brief Compatibility wrapper for getIntegrationTime()
     */
    float getIntegrationTime() { return integrationTime(); }

    /**
     * @brief Compatibility wrapper for readAll()
     */
    void readAll(uint16_t &x, uint16_t &y, uint16_t &z, uint16_t &ir1, uint16_t &ir2) {
      RawData const DATA = raw();
      x = DATA.X;
      y = DATA.Y;
      z = DATA.Z;
      ir1 = DATA.IR1;
      ir2 = DATA.IR2;
    }

    /**
     * @brief Compatibility wrapper for getX()
     */
    uint16_t getX() { return raw().X; }

    /**
     * @brief Compatibility wrapper for getY()
     */
    uint16_t getY() { return raw().Y; }

    /**
     * @brief Compatibility wrapper for getZ()
     */
    uint16_t getZ() { return raw().Z; }

    /**
     * @brief Compatibility wrapper for getIR1()
     */
    uint16_t getIR1() { return raw().IR1; }

    /**
     * @brief Compatibility wrapper for getIR2()
     */
    uint16_t getIR2() { return raw().IR2; }

    /**
     * @brief Compatibility wrapper for powerOn()
     */
    void powerOn(bool on) { power(on); }

    /**
     * @brief Compatibility wrapper for enableALS()
     */
    void enableALS(bool enable) {
      if (enable) {
        mode(ALS);
      } else {
        mode(IDLE);
      }
    }

    /**
     * @brief Compatibility wrapper for autoGain with old signature
     */
    bool autoGain(uint16_t targetY, OldGain initGain, float /*maxIntTimeMs*/) {
      // Convert old gain to new gain
      TCS3430AutoGain::Gain newGain = X16;  // default
      switch (initGain) {
        case OldGain::GAIN_1X:
          newGain = X01;
          break;
        case OldGain::GAIN_4X:
          newGain = X04;
          break;
        case OldGain::GAIN_16X:
          newGain = X16;
          break;
        case OldGain::GAIN_64X:
          newGain = X64;
          break;
        default:
          newGain = X16;
          break;
      }
      return autoGain(targetY, newGain);
    }

    // DFRobot compatibility methods
    uint16_t getXData() { return getX(); }
    uint16_t getYData() { return getY(); }
    uint16_t getZData() { return getZ(); }
    uint16_t getIR1Data() { return getIR1(); }
    uint16_t getIR2Data() { return getIR2(); }

    void setALSGain(int gain) {
        switch (gain) {
            case 0: setGain(OldGain::GAIN_1X); break;
            case 1: setGain(OldGain::GAIN_4X); break;
            case 2: setGain(OldGain::GAIN_16X); break;
            case 3: setGain(OldGain::GAIN_64X); break;
            default: setGain(OldGain::GAIN_16X); break;
        }
    }

    void setHighGAIN(bool enable) {
        // New library doesn't support 128x, so just use 64x
        if (enable) {
          setGain(OldGain::GAIN_64X);
        }
    }

    // Placeholder methods for unsupported features
    void enableWait(bool enable) { /* Not implemented in new library */ }
    void enableWaitLong(bool enable) { /* Not implemented in new library */ }
    void setWaitTime(float ms) { wait(ms); }
    float getWaitTime() { return wait(); }
    static bool isWaitEnabled() {
      return false;
    }
    static bool getWaitLong() {
      return false;
    }
    void setAutoZeroMode(uint8_t mode) { /* Not implemented in new library */ }
    static uint8_t getAutoZeroMode() {
      return 1;
    }
    void setAutoZeroNTHIteration(uint8_t nth) { /* Not implemented in new library */ }
    static uint8_t getAutoZeroNTHIteration() {
      return 0;
    }
    void setInterruptPersistence(uint8_t pers) { persistence(pers); }
    uint8_t getInterruptPersistence() { return persistence(); }
    void setInterruptThresholds(uint16_t low, uint16_t high) { interruptThresholds(low, high); }
    uint16_t getLowInterruptThreshold() { return lowInterruptThreshold(); }
    uint16_t getHighInterruptThreshold() { return highInterruptThreshold(); }
    void enableALSInterrupt(bool enable) { /* Not implemented in new library */ }
    static bool isALSInterruptEnabled() {
      return false;
    }
    void enableSaturationInterrupt(bool enable) { /* Not implemented in new library */ }
    static bool isSaturationInterruptEnabled() {
      return false;
    }
    void clearInterrupt() { /* Not implemented in new library */ }
    bool getInterruptStatus() { return interrupt(); }
    static bool getSaturationStatus() {
      return false;
    }  // New library doesn't have saturation detection
    static bool dataReady() {
      return true;
    }  // Always ready in new library
    static bool isPowerOn() {
      return true;
    }
    static bool isALSEnabled() {
      return true;
    }
    static uint16_t getMaxCount() {
      return 65535;
    }

private:
 TwoWire *_wire{nullptr};
 uint8_t _addr{TCS3430_ADDRESS};
 float _ga{1.0f};  // Glass attenuation
 bool _calcEnabled{true};

 // Advanced color science data
 ColorScience::CalibrationData _calibData;

 // Auto-gain configuration list (adapted for TCS3430 gains, sorted from high to low sensitivity)
 struct AgcT {
   Gain g;
   uint8_t atime;  // ATIME register value (0-255)
   uint16_t mincnt;
   uint16_t maxcnt;
 };
 static const AgcT AGC_LIST[];
 static const uint8_t AGC_LIST_SIZE = 16;

 // Helper functions
 void write8(uint8_t reg, uint8_t val);
 uint8_t read8(uint8_t reg);
 uint16_t read16(uint8_t reg_low);
};

#endif  // TCS3430_AUTO_GAIN_H
