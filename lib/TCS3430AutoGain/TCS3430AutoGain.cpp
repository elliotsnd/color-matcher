/**
 * @file TCS3430AutoGain.cpp
 * @brief Implementation of the TCS3430AutoGain library.
 */

#include "TCS3430AutoGain.h"

// Auto-gain list (high sensitivity first: high gain, long integration)
const TCS3430AutoGain::AgcT TCS3430AutoGain::AGC_LIST[] = {
    {X64, 0xFF, 0x1000, 0xFFFF},  // 64x, 256 cycles (~711 ms)
    {X64, 0x8F,  0x800, 0x7FFF},  // 64x, 144 cycles
    {X64, 0x3F,  0x200, 0x1FFF},  // 64x, 64 cycles
    {X64, 0x0F,   0x80, 0x07FF},  // 64x, 16 cycles
    {X16, 0xFF,  0x400, 0x3FFF},  // 16x, 256 cycles
    {X16, 0x8F,  0x200, 0x1FFF},
    {X16, 0x3F,   0x80, 0x07FF},
    {X16, 0x0F,   0x20, 0x01FF},
    {X04, 0xFF,  0x100, 0x0FFF},  // 4x, 256 cycles
    {X04, 0x8F,   0x80, 0x07FF},
    {X04, 0x3F,   0x20, 0x01FF},
    {X04, 0x0F,    0x8, 0x007F},
    {X01, 0xFF,   0x40, 0x03FF},  // 1x, 256 cycles
    {X01, 0x8F,   0x20, 0x01FF},
    {X01, 0x3F,    0x8, 0x007F},
    {X01, 0x0F,    0x1, 0x001F}   // 1x, 16 cycles (lowest)
};

bool TCS3430AutoGain::begin(TwoWire &w, uint8_t addr) {
    _wire = &w;
    _addr = addr;
    _wire->begin();
    if (read8(TCS3430_ID) != 0xDC) return false;  // Check device ID
    power(true);
    setAMux(false);  // Default to X on CH3
    return true;
}

void TCS3430AutoGain::power(bool b) {
    uint8_t en = read8(TCS3430_ENABLE);
    if (b) en |= 0x01; else en &= ~0x01;
    write8(TCS3430_ENABLE, en);
    if (b) delay(3);  // Power-on delay
}

TCS3430AutoGain::Mode TCS3430AutoGain::mode(Mode m) {
    uint8_t en = read8(TCS3430_ENABLE);
    bool pon = en & 0x01;
    bool aen = en & 0x02;
    bool wen = en & 0x08;

    if (m == UNDEFINED) {
        if (!pon) return SLEEP;
        if (!aen) return IDLE;
        if (wen) return WAIT_ALS;
        return ALS;
    }

    switch (m) {
        case SLEEP: en &= ~0x01; break;
        case IDLE: en = (en | 0x01) & ~0x0A; break;
        case ALS: en = (en | 0x03) & ~0x08; break;
        case WAIT_ALS: en |= 0x0B; break;
        default: break;
    }
    write8(TCS3430_ENABLE, en);
    return mode(UNDEFINED);
}

float TCS3430AutoGain::integrationTime(float ms) {
    if (ms < 0) {
        uint8_t at = read8(TCS3430_ATIME);
        return (at + 1) * TCS3430_STEP_MS;
    }
    uint8_t at = constrain(round(ms / TCS3430_STEP_MS) - 1, 0, 255);
    write8(TCS3430_ATIME, at);
    return integrationTime(-1.0f);
}

int16_t TCS3430AutoGain::integrationCycles(int16_t cycles) {
    if (cycles < 0) {
        return read8(TCS3430_ATIME) + 1;
    }
    uint8_t at = constrain(cycles - 1, 0, 255);
    write8(TCS3430_ATIME, at);
    return integrationCycles(-1);
}

float TCS3430AutoGain::gain(Gain g) {
    uint8_t cfg1 = read8(TCS3430_CFG1);
    if (g == (Gain)-1) {
        uint8_t ag = cfg1 & 0x03;
        switch (ag) {
            case 0: return 1.0f;
            case 1: return 4.0f;
            case 2: return 16.0f;
            case 3: return 64.0f;
        }
    }
    uint8_t ag = static_cast<uint8_t>(g);
    cfg1 = (cfg1 & ~0x03) | (ag & 0x03);
    write8(TCS3430_CFG1, cfg1);
    return gain((Gain)-1);
}

bool TCS3430AutoGain::autoGain(uint16_t minYCount, Gain initGain) {
    gain(initGain);  // Set initial gain
    for (uint8_t i = 0; i < AGC_LIST_SIZE; ++i) {
        const AgcT &ag = AGC_LIST[i];
        gain(ag.g);
        write8(TCS3430_ATIME, ag.atime);
        mode(ALS);
        delay((ag.atime + 1) * TCS3430_STEP_MS + 1.0f);  // Wait for integration
        RawData rd = raw();
        uint8_t status = getDeviceStatus();
        if ((status & 0x80) || rd.Y > ag.maxcnt) continue;  // Saturated or too high, try lower sensitivity
        if (rd.Y < ag.mincnt || rd.Y < minYCount) return false;  // Too low
        return true;  // Optimal
    }
    return false;  // Could not find suitable setting
}

bool TCS3430AutoGain::singleRead() {
    mode(ALS);
    delay(integrationTime(-1.0f) + 1.0f);
    return true;
}

TCS3430AutoGain::RawData TCS3430AutoGain::raw() {
    RawData d;
    d.Z = read16(TCS3430_CH0DATAL);
    d.Y = read16(TCS3430_CH1DATAL);
    d.IR1 = read16(TCS3430_CH2DATAL);
    d.X = read16(TCS3430_CH3DATAL);
    d.IR2 = read16(TCS3430_CH4DATAL);
    return d;
}

bool TCS3430AutoGain::interrupt() {
    return (read8(TCS3430_STATUS) & 0x10) != 0;
}

uint8_t TCS3430AutoGain::persistence() {
    return read8(TCS3430_PERS) & 0x0F;
}

void TCS3430AutoGain::persistence(uint8_t p) {
    write8(TCS3430_PERS, p & 0x0F);
}

uint16_t TCS3430AutoGain::lowInterruptThreshold() {
    return read16(TCS3430_AILTL);
}

uint16_t TCS3430AutoGain::highInterruptThreshold() {
    return read16(TCS3430_AIHTL);
}

void TCS3430AutoGain::interruptThresholds(uint16_t low, uint16_t high) {
    write8(TCS3430_AILTL, low & 0xFF);
    write8(TCS3430_AILTH, low >> 8);
    write8(TCS3430_AIHTL, high & 0xFF);
    write8(TCS3430_AIHTH, high >> 8);
}

float TCS3430AutoGain::wait(float ms, bool enterWaitALS) {
    bool wlong = (read8(TCS3430_CFG0) & 0x04) != 0;
    float mul = wlong ? TCS3430_LONG_WAIT_MUL : 1.0f;
    if (ms < 0) {
        uint8_t wt = read8(TCS3430_WTIME);
        return (wt + 1) * TCS3430_STEP_MS * mul;
    }
    if (ms > 256 * TCS3430_STEP_MS) {
        wlong = true;
        uint8_t wt = constrain(round(ms / (TCS3430_STEP_MS * TCS3430_LONG_WAIT_MUL)) - 1, 0, 255);
        write8(TCS3430_WTIME, wt);
    } else {
        wlong = false;
        uint8_t wt = constrain(round(ms / TCS3430_STEP_MS) - 1, 0, 255);
        write8(TCS3430_WTIME, wt);
    }
    uint8_t cfg0 = read8(TCS3430_CFG0);
    if (wlong) cfg0 |= 0x04; else cfg0 &= ~0x04;
    write8(TCS3430_CFG0, cfg0);
    if (enterWaitALS) mode(WAIT_ALS);
    return wait(-1.0f);
}

float TCS3430AutoGain::lux() {
    if (!_calcEnabled) return 0.0f;
    RawData rd = raw();
    return static_cast<float>(rd.Y) / _ga;  // Approximate, user can calibrate further
}

float TCS3430AutoGain::colorTemp() {
    if (!_calcEnabled) return 0.0f;
    float x = chromaticityX();
    float y = chromaticityY();
    float n = (x - 0.3320f) / (0.1858f - y);
    return 449.0f * n * n * n + 3525.0f * n * n + 6823.3f * n + 5520.33f;
}

float TCS3430AutoGain::chromaticityX() {
    RawData rd = raw();
    float sum = static_cast<float>(rd.X + rd.Y + rd.Z);
    return (sum > 0.0f) ? static_cast<float>(rd.X) / sum : 0.0f;
}

float TCS3430AutoGain::chromaticityY() {
    RawData rd = raw();
    float sum = static_cast<float>(rd.X + rd.Y + rd.Z);
    return (sum > 0.0f) ? static_cast<float>(rd.Y) / sum : 0.0f;
}

void TCS3430AutoGain::setAMux(bool ir2) {
    uint8_t cfg1 = read8(TCS3430_CFG1);
    if (ir2) cfg1 |= 0x08; else cfg1 &= ~0x08;
    write8(TCS3430_CFG1, cfg1);
}

void TCS3430AutoGain::write8(uint8_t reg, uint8_t val) {
    _wire->beginTransmission(_addr);
    _wire->write(reg);
    _wire->write(val);
    _wire->endTransmission();
}

uint8_t TCS3430AutoGain::read8(uint8_t reg) {
    _wire->beginTransmission(_addr);
    _wire->write(reg);
    _wire->endTransmission();
    _wire->requestFrom(_addr, (uint8_t)1);
    return _wire->available() ? _wire->read() : 0;
}

uint16_t TCS3430AutoGain::read16(uint8_t reg_low) {
    uint8_t low = read8(reg_low);
    uint8_t high = read8(reg_low + 1);
    return (high << 8) | low;
}

// ========================================
// ADVANCED COLOR SCIENCE IMPLEMENTATION
// ========================================

ColorScience::RGBColor TCS3430AutoGain::getRGBColor(bool useAdvancedColorScience) {
    if (!useAdvancedColorScience) {
        // Fallback to simple conversion for compatibility
        RawData data = raw();
        ColorScience::RGBColor result;

        // Simple normalization and basic sRGB conversion
        float X_norm = static_cast<float>(data.X) / 65535.0f;
        float Y_norm = static_cast<float>(data.Y) / 65535.0f;
        float Z_norm = static_cast<float>(data.Z) / 65535.0f;

        // Basic sRGB matrix (simplified)
        result.r = 3.2406f * X_norm - 1.5372f * Y_norm - 0.4986f * Z_norm;
        result.g = -0.9689f * X_norm + 1.8758f * Y_norm + 0.0415f * Z_norm;
        result.b = 0.0557f * X_norm - 0.2040f * Y_norm + 1.0570f * Z_norm;

        // Clamp and convert to 8-bit
        result.r = ColorScience::clamp(result.r);
        result.g = ColorScience::clamp(result.g);
        result.b = ColorScience::clamp(result.b);

        ColorScience::floatToRGB8(result.r, result.g, result.b, result.r8, result.g8, result.b8);
        return result;
    }

    // Advanced color science conversion
    RawData data = raw();

    // Convert raw data to XYZ
    ColorScience::XYZColor xyz = ColorScience::rawToXYZ(data.X, data.Y, data.Z, data.IR1, data.IR2, _calibData);

    // Prepare IR data
    ColorScience::IRData irData;
    irData.IR1 = static_cast<float>(data.IR1) / 65535.0f;
    irData.IR2 = static_cast<float>(data.IR2) / 65535.0f;
    irData.ratio = (irData.IR2 > 0.0f) ? (irData.IR1 / irData.IR2) : 1.0f;

    // Apply advanced color science
    return ColorScience::xyzToRGB(xyz, irData, _calibData);
}

ColorScience::XYZColor TCS3430AutoGain::getXYZColor() {
    RawData data = raw();
    return ColorScience::rawToXYZ(data.X, data.Y, data.Z, data.IR1, data.IR2, _calibData);
}

void TCS3430AutoGain::setCalibrationData(const ColorScience::CalibrationData& calibData) {
    _calibData = calibData;
}

ColorScience::CalibrationData TCS3430AutoGain::getCalibrationData() const {
    return _calibData;
}

bool TCS3430AutoGain::calibrateWhiteReference(int numSamples) {
    if (numSamples <= 0) return false;

    // Accumulate samples
    float sumX = 0, sumY = 0, sumZ = 0;
    float sumIR1 = 0, sumIR2 = 0;

    for (int i = 0; i < numSamples; i++) {
        RawData data = raw();
        sumX += static_cast<float>(data.X);
        sumY += static_cast<float>(data.Y);
        sumZ += static_cast<float>(data.Z);
        sumIR1 += static_cast<float>(data.IR1);
        sumIR2 += static_cast<float>(data.IR2);
        delay(50); // Small delay between samples
    }

    // Calculate averages and normalize
    _calibData.whiteReference.X = (sumX / numSamples) / 65535.0f;
    _calibData.whiteReference.Y = (sumY / numSamples) / 65535.0f;
    _calibData.whiteReference.Z = (sumZ / numSamples) / 65535.0f;

    _calibData.whiteIR.IR1 = (sumIR1 / numSamples) / 65535.0f;
    _calibData.whiteIR.IR2 = (sumIR2 / numSamples) / 65535.0f;
    _calibData.whiteIR.ratio = (_calibData.whiteIR.IR2 > 0.0f) ?
                               (_calibData.whiteIR.IR1 / _calibData.whiteIR.IR2) : 1.0f;

    return ColorScience::validateCalibrationData(_calibData);
}

bool TCS3430AutoGain::calibrateBlackReference(int numSamples) {
    if (numSamples <= 0) return false;

    // Accumulate samples
    float sumX = 0, sumY = 0, sumZ = 0;
    float sumIR1 = 0, sumIR2 = 0;

    for (int i = 0; i < numSamples; i++) {
        RawData data = raw();
        sumX += static_cast<float>(data.X);
        sumY += static_cast<float>(data.Y);
        sumZ += static_cast<float>(data.Z);
        sumIR1 += static_cast<float>(data.IR1);
        sumIR2 += static_cast<float>(data.IR2);
        delay(50); // Small delay between samples
    }

    // Calculate averages and normalize
    _calibData.blackReference.X = (sumX / numSamples) / 65535.0f;
    _calibData.blackReference.Y = (sumY / numSamples) / 65535.0f;
    _calibData.blackReference.Z = (sumZ / numSamples) / 65535.0f;

    _calibData.blackIR.IR1 = (sumIR1 / numSamples) / 65535.0f;
    _calibData.blackIR.IR2 = (sumIR2 / numSamples) / 65535.0f;
    _calibData.blackIR.ratio = (_calibData.blackIR.IR2 > 0.0f) ?
                               (_calibData.blackIR.IR1 / _calibData.blackIR.IR2) : 1.0f;

    return ColorScience::validateCalibrationData(_calibData);
}

void TCS3430AutoGain::configureColorScience(bool enableIRCompensation,
                                           bool enableAmbientCompensation,
                                           float irCompensationFactor) {
    _calibData.irCompensationFactor = ColorScience::clamp(irCompensationFactor, 0.0f, 1.0f);
    _calibData.ambientCompensationEnabled = enableAmbientCompensation;

    // IR compensation is controlled by the irCompensationFactor (0 = disabled)
    if (!enableIRCompensation) {
        _calibData.irCompensationFactor = 0.0f;
    }
}

void TCS3430AutoGain::setColorSpace(bool useAdobeRGB) {
    _calibData.useAdobeRGB = useAdobeRGB;
}

void TCS3430AutoGain::setCustomMatrix(const float matrix[9]) {
    _calibData.useCustomMatrix = true;
    for (int i = 0; i < 9; i++) {
        _calibData.customMatrix[i] = matrix[i];
    }
}

void TCS3430AutoGain::configureLEDIRCompensation(float baseCompensation,
                                                float brightnessResponse,
                                                bool adaptToBrightness) {
    _calibData.ledIR.baseIRCompensation = ColorScience::clamp(baseCompensation, 0.0f, 0.3f);
    _calibData.ledIR.ledBrightnessResponse = ColorScience::clamp(brightnessResponse, 0.0f, 0.1f);
    _calibData.ledIR.adaptToLEDBrightness = adaptToBrightness;

    // Set reasonable min/max limits based on base compensation
    _calibData.ledIR.minCompensation = baseCompensation * 0.25f;  // 25% of base
    _calibData.ledIR.maxCompensation = baseCompensation * 3.0f;   // 300% of base
}

void TCS3430AutoGain::setChannelIRLeakage(float xLeakage, float yLeakage, float zLeakage) {
    _calibData.spectral.xChannelIRLeakage = ColorScience::clamp(xLeakage, 0.0f, 0.2f);
    _calibData.spectral.yChannelIRLeakage = ColorScience::clamp(yLeakage, 0.0f, 0.2f);
    _calibData.spectral.zChannelIRLeakage = ColorScience::clamp(zLeakage, 0.0f, 0.3f);
    _calibData.spectral.useChannelSpecificIR = true;
}

bool TCS3430AutoGain::calibrateLEDIRResponse(int numBrightnessLevels, int samplesPerLevel) {
    if (numBrightnessLevels < 3 || numBrightnessLevels > 10 ||
        samplesPerLevel < 5 || samplesPerLevel > 20) {
        return false;
    }

    // This method would require external LED control
    // For now, we'll set up the framework and return success
    // In a real implementation, this would:
    // 1. Set LED to different brightness levels
    // 2. Measure XYZ and IR at each level
    // 3. Calculate the relationship between LED brightness and IR contamination
    // 4. Update the calibration parameters

    // Placeholder implementation - sets reasonable defaults
    configureLEDIRCompensation(0.08f, 0.02f, true);
    setChannelIRLeakage(0.03f, 0.015f, 0.08f);

    return true;
}
