/**
 * @file TCS3430AutoGain.cpp
 * @brief Implementation of the TCS3430AutoGain library.
 */

#include "TCS3430AutoGain.h"
#include <functional>
#include <vector>

// Auto-gain list (high sensitivity first: high gain, long integration)
const std::array<TCS3430AutoGain::AgcT, TCS3430AutoGain::AGC_LIST_SIZE> TCS3430AutoGain::AGC_LIST = {{
    {TCS3430AutoGain::Gain::GAIN_64X, 0xFF, 0x1000, 0xFFFF},
    {TCS3430AutoGain::Gain::GAIN_64X, 0x8F,  0x800, 0x7FFF},
    {TCS3430AutoGain::Gain::GAIN_64X, 0x3F,  0x200, 0x1FFF},
    {TCS3430AutoGain::Gain::GAIN_64X, 0x0F,   0x80, 0x07FF},
    {TCS3430AutoGain::Gain::GAIN_16X, 0xFF,  0x400, 0x3FFF},
    {TCS3430AutoGain::Gain::GAIN_16X, 0x8F,  0x200, 0x1FFF},
    {TCS3430AutoGain::Gain::GAIN_16X, 0x3F,   0x80, 0x07FF},
    {TCS3430AutoGain::Gain::GAIN_16X, 0x0F,   0x20, 0x01FF},
    {TCS3430AutoGain::Gain::GAIN_4X, 0xFF,  0x100, 0x0FFF},
    {TCS3430AutoGain::Gain::GAIN_4X, 0x8F,   0x80, 0x07FF},
    {TCS3430AutoGain::Gain::GAIN_4X, 0x3F,   0x20, 0x01FF},
    {TCS3430AutoGain::Gain::GAIN_4X, 0x0F,    0x8, 0x007F},
    {TCS3430AutoGain::Gain::GAIN_1X, 0xFF,   0x40, 0x03FF},
    {TCS3430AutoGain::Gain::GAIN_1X, 0x8F,   0x20, 0x01FF},
    {TCS3430AutoGain::Gain::GAIN_1X, 0x3F,    0x8, 0x007F},
    {TCS3430AutoGain::Gain::GAIN_1X, 0x0F,    0x1, 0x001F}
}};

bool TCS3430AutoGain::begin(TwoWire &w, uint8_t addr) {
    _wire = &w;
    _addr = addr;
    _wire->begin();
    if (read8(static_cast<uint8_t>(TCS3430Register::ID)) != 0xDC) return false;
    power(true);
    setAMux(false);
    return true;
}

void TCS3430AutoGain::power(bool b) {
    uint8_t en = read8(static_cast<uint8_t>(TCS3430Register::ENABLE));
    if (b) en |= 0x01; else en &= ~0x01;
    write8(static_cast<uint8_t>(TCS3430Register::ENABLE), en);
    if (b) delay(3);
}

TCS3430AutoGain::Mode TCS3430AutoGain::mode(Mode m) {
    uint8_t en = read8(static_cast<uint8_t>(TCS3430Register::ENABLE));
    bool pon = en & 0x01;
    bool aen = en & 0x02;
    bool wen = en & 0x08;

    if (m == Mode::UNDEFINED) {
        if (!pon) return Mode::SLEEP;
        if (!aen) return Mode::IDLE;
        if (wen) return Mode::WAIT_ALS;
        return Mode::ALS;
    }

    switch (m) {
        case Mode::SLEEP:    en &= ~0x01; break;
        case Mode::IDLE:     en = (en | 0x01) & ~0x0A; break;
        case Mode::ALS:      en = (en | 0x03) & ~0x08; break;
        case Mode::WAIT_ALS: en |= 0x0B; break;
        default: break;
    }
    write8(static_cast<uint8_t>(TCS3430Register::ENABLE), en);
    return mode(Mode::UNDEFINED);
}

float TCS3430AutoGain::integrationTime(float ms) {
    if (ms < 0) {
        uint8_t at = read8(static_cast<uint8_t>(TCS3430Register::ATIME));
        return (at + 1) * TCS3430_STEP_MS;
    }
    uint8_t at = constrain(round(ms / TCS3430_STEP_MS) - 1, 0, 255);
    write8(static_cast<uint8_t>(TCS3430Register::ATIME), at);
    return integrationTime(-1.0f);
}

int16_t TCS3430AutoGain::integrationCycles(int16_t cycles) {
    if (cycles < 0) {
        return read8(static_cast<uint8_t>(TCS3430Register::ATIME)) + 1;
    }
    uint8_t at = constrain(cycles - 1, 0, 255);
    write8(static_cast<uint8_t>(TCS3430Register::ATIME), at);
    return integrationCycles(-1);
}

float TCS3430AutoGain::gain(Gain g) {
    uint8_t cfg1 = read8(static_cast<uint8_t>(TCS3430Register::CFG1));
    if (g == static_cast<Gain>(-1)) {
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
    write8(static_cast<uint8_t>(TCS3430Register::CFG1), cfg1);
    return gain(static_cast<Gain>(-1));
}

bool TCS3430AutoGain::autoGain(uint16_t minYCount, Gain initGain) {
    gain(initGain);
    for (uint8_t i = 0; i < AGC_LIST_SIZE; ++i) {
        const AgcT &ag = AGC_LIST[i];
        gain(ag.g);
        write8(static_cast<uint8_t>(TCS3430Register::ATIME), ag.atime);
        mode(Mode::ALS);
        delay((ag.atime + 1) * TCS3430_STEP_MS + 1.0f);
        RawData rd = raw();
        uint8_t status = getDeviceStatus();
        if ((status & 0x80) || rd.Y > ag.maxcnt) continue;
        if (rd.Y < ag.mincnt && rd.Y < minYCount) continue;
        return true;
    }
    return false;
}

bool TCS3430AutoGain::singleRead() {
    mode(Mode::ALS);
    delay(integrationTime(-1.0f) + 1.0f);
    return true;
}

TCS3430AutoGain::RawData TCS3430AutoGain::raw() {
    RawData d;
    d.Z = read16(static_cast<uint8_t>(TCS3430Register::CH0DATAL));
    d.Y = read16(static_cast<uint8_t>(TCS3430Register::CH1DATAL));
    d.IR1 = read16(static_cast<uint8_t>(TCS3430Register::CH2DATAL));
    d.X = read16(static_cast<uint8_t>(TCS3430Register::CH3DATAL));
    d.IR2 = read16(static_cast<uint8_t>(TCS3430Register::CH4DATAL));
    return d;
}

bool TCS3430AutoGain::interrupt() {
    return (read8(static_cast<uint8_t>(TCS3430Register::STATUS)) & 0x10) != 0;
}

uint8_t TCS3430AutoGain::persistence() {
    return read8(static_cast<uint8_t>(TCS3430Register::PERS)) & 0x0F;
}

void TCS3430AutoGain::persistence(uint8_t p) {
    write8(static_cast<uint8_t>(TCS3430Register::PERS), p & 0x0F);
}

uint16_t TCS3430AutoGain::lowInterruptThreshold() {
    return read16(static_cast<uint8_t>(TCS3430Register::AILTL));
}

uint16_t TCS3430AutoGain::highInterruptThreshold() {
    return read16(static_cast<uint8_t>(TCS3430Register::AIHTL));
}

void TCS3430AutoGain::interruptThresholds(uint16_t low, uint16_t high) {
    write8(static_cast<uint8_t>(TCS3430Register::AILTL), low & 0xFF);
    write8(static_cast<uint8_t>(TCS3430Register::AILTH), low >> 8);
    write8(static_cast<uint8_t>(TCS3430Register::AIHTL), high & 0xFF);
    write8(static_cast<uint8_t>(TCS3430Register::AIHTH), high >> 8);
}

float TCS3430AutoGain::wait(float ms, bool enterWaitALS) {
    bool wlong = (read8(static_cast<uint8_t>(TCS3430Register::CFG0)) & 0x04) != 0;
    float mul = wlong ? TCS3430_LONG_WAIT_MUL : 1.0f;
    if (ms < 0) {
        uint8_t wt = read8(static_cast<uint8_t>(TCS3430Register::WTIME));
        return (wt + 1) * TCS3430_STEP_MS * mul;
    }
    if (ms > 256 * TCS3430_STEP_MS) {
        wlong = true;
        uint8_t wt = constrain(round(ms / (TCS3430_STEP_MS * TCS3430_LONG_WAIT_MUL)) - 1, 0, 255);
        write8(static_cast<uint8_t>(TCS3430Register::WTIME), wt);
    } else {
        wlong = false;
        uint8_t wt = constrain(round(ms / TCS3430_STEP_MS) - 1, 0, 255);
        write8(static_cast<uint8_t>(TCS3430Register::WTIME), wt);
    }
    uint8_t cfg0 = read8(static_cast<uint8_t>(TCS3430Register::CFG0));
    if (wlong) cfg0 |= 0x04; else cfg0 &= ~0x04;
    write8(static_cast<uint8_t>(TCS3430Register::CFG0), cfg0);
    if (enterWaitALS) mode(Mode::WAIT_ALS);
    return wait(-1.0f);
}

float TCS3430AutoGain::lux() {
    if (!_calcEnabled) return 0.0f;
    RawData rd = raw();
    return static_cast<float>(rd.Y) / _ga;
}

float TCS3430AutoGain::colorTemp() {
    if (!_calcEnabled) return 0.0f;
    float x = chromaticityX();
    float y = chromaticityY();
    if (y == 0.0f) return 0.0f;
    float n = (x - 0.3320f) / (0.1858f - y);
    return 449.0f * pow(n, 3) + 3525.0f * pow(n, 2) + 6823.3f * n + 5520.33f;
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
    uint8_t cfg1 = read8(static_cast<uint8_t>(TCS3430Register::CFG1));
    if (ir2) cfg1 |= 0x08; else cfg1 &= ~0x08;
    write8(static_cast<uint8_t>(TCS3430Register::CFG1), cfg1);
}

// ========================================
// ADVANCED COLOR SCIENCE IMPLEMENTATION
// ========================================

ColorScience::RGBColor TCS3430AutoGain::getRGBColor(bool useAdvancedColorScience) {
    if (!useAdvancedColorScience) {
        RawData data = raw();
        ColorScience::RGBColor result;
        float X_norm = static_cast<float>(data.X) / 65535.0f;
        float Y_norm = static_cast<float>(data.Y) / 65535.0f;
        float Z_norm = static_cast<float>(data.Z) / 65535.0f;
        result.r = 3.2406f * X_norm - 1.5372f * Y_norm - 0.4986f * Z_norm;
        result.g = -0.9689f * X_norm + 1.8758f * Y_norm + 0.0415f * Z_norm;
        result.b = 0.0557f * X_norm - 0.2040f * Y_norm + 1.0570f * Z_norm;
        result.r = ColorScience::clamp(result.r);
        result.g = ColorScience::clamp(result.g);
        result.b = ColorScience::clamp(result.b);
        ColorScience::floatToRGB8(result.r, result.g, result.b, result.r8, result.g8, result.b8);
        return result;
    }
    RawData data = raw();
    ColorScience::XYZColor xyz = ColorScience::rawToXYZ(data.X, data.Y, data.Z, data.IR1, data.IR2, _calibData);
    ColorScience::IRData irData;
    irData.IR1 = static_cast<float>(data.IR1) / 65535.0f;
    irData.IR2 = static_cast<float>(data.IR2) / 65535.0f;
    irData.ratio = (irData.IR2 > 0.0f) ? (irData.IR1 / irData.IR2) : 1.0f;
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
    double sumX = 0, sumY = 0, sumZ = 0;
    double sumIR1 = 0, sumIR2 = 0;
    for (int i = 0; i < numSamples; i++) {
        RawData data = raw();
        sumX += data.X; sumY += data.Y; sumZ += data.Z;
        sumIR1 += data.IR1; sumIR2 += data.IR2;
        delay(50);
    }
    _calibData.whiteReference.X = (sumX / numSamples) / 65535.0f;
    _calibData.whiteReference.Y = (sumY / numSamples) / 65535.0f;
    _calibData.whiteReference.Z = (sumZ / numSamples) / 65535.0f;
    _calibData.whiteIR.IR1 = (sumIR1 / numSamples) / 65535.0f;
    _calibData.whiteIR.IR2 = (sumIR2 / numSamples) / 65535.0f;
    _calibData.whiteIR.ratio = (_calibData.whiteIR.IR2 > 1e-5) ? (_calibData.whiteIR.IR1 / _calibData.whiteIR.IR2) : 1.0f;
    return ColorScience::validateCalibrationData(_calibData);
}

bool TCS3430AutoGain::calibrateBlackReference(int numSamples) {
    if (numSamples <= 0) return false;
    double sumX = 0, sumY = 0, sumZ = 0;
    double sumIR1 = 0, sumIR2 = 0;
    for (int i = 0; i < numSamples; i++) {
        RawData data = raw();
        sumX += data.X; sumY += data.Y; sumZ += data.Z;
        sumIR1 += data.IR1; sumIR2 += data.IR2;
        delay(50);
    }
    _calibData.blackReference.X = (sumX / numSamples) / 65535.0f;
    _calibData.blackReference.Y = (sumY / numSamples) / 65535.0f;
    _calibData.blackReference.Z = (sumZ / numSamples) / 65535.0f;
    _calibData.blackIR.IR1 = (sumIR1 / numSamples) / 65535.0f;
    _calibData.blackIR.IR2 = (sumIR2 / numSamples) / 65535.0f;
    _calibData.blackIR.ratio = (_calibData.blackIR.IR2 > 1e-5) ? (_calibData.blackIR.IR1 / _calibData.blackIR.IR2) : 1.0f;
    return ColorScience::validateCalibrationData(_calibData);
}

void TCS3430AutoGain::configureColorScience(bool enableIRCompensation, bool enableAmbientCompensation, float irCompensationFactor) {
    _calibData.ambientCompensationEnabled = enableAmbientCompensation;
    _calibData.irCompensationFactor = enableIRCompensation ? ColorScience::clamp(irCompensationFactor, 0.0f, 1.0f) : 0.0f;
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

void TCS3430AutoGain::configureLEDIRCompensation(float baseCompensation, float brightnessResponse, bool adaptToBrightness) {
    _calibData.ledIR.baseIRCompensation = ColorScience::clamp(baseCompensation, 0.0f, 0.3f);
    _calibData.ledIR.ledBrightnessResponse = ColorScience::clamp(brightnessResponse, 0.0f, 0.1f);
    _calibData.ledIR.adaptToLEDBrightness = adaptToBrightness;
    _calibData.ledIR.minCompensation = baseCompensation * 0.25f;
    _calibData.ledIR.maxCompensation = baseCompensation * 3.0f;
}

void TCS3430AutoGain::setChannelIRLeakage(float xLeakage, float yLeakage, float zLeakage) {
    _calibData.spectral.xChannelIRLeakage = ColorScience::clamp(xLeakage, 0.0f, 0.2f);
    _calibData.spectral.yChannelIRLeakage = ColorScience::clamp(yLeakage, 0.0f, 0.2f);
    _calibData.spectral.zChannelIRLeakage = ColorScience::clamp(zLeakage, 0.0f, 0.3f);
    _calibData.spectral.useChannelSpecificIR = true;
}

bool TCS3430AutoGain::calibrateLEDIRResponse(std::function<void(uint8_t)> setLedBrightness, int numBrightnessLevels, int samplesPerLevel) {
    if (numBrightnessLevels < 3 || numBrightnessLevels > 10 || samplesPerLevel < 5 || samplesPerLevel > 20) return false;

    setLedBrightness(0);
    delay(200);

    double ambientIR1 = 0, ambientIR2 = 0;
    for (int i = 0; i < samplesPerLevel; i++) {
        RawData data = raw();
        ambientIR1 += data.IR1;
        ambientIR2 += data.IR2;
        delay(20);
    }
    ambientIR1 /= samplesPerLevel;
    ambientIR2 /= samplesPerLevel;
    float ambientIR_avg = (ambientIR1 + ambientIR2) / 2.0f;

    std::vector<std::pair<float, float>> irDataPoints;
    for (int i = 1; i <= numBrightnessLevels; i++) {
        uint8_t brightness = map(i, 0, numBrightnessLevels, 50, 255);
        setLedBrightness(brightness);
        delay(100);

        double levelIR1 = 0, levelIR2 = 0;
        for (int s = 0; s < samplesPerLevel; s++) {
            RawData data = raw();
            levelIR1 += data.IR1;
            levelIR2 += data.IR2;
            delay(20);
        }
        levelIR1 /= samplesPerLevel;
        levelIR2 /= samplesPerLevel;

        float contamination = ((levelIR1 + levelIR2) / 2.0f) - ambientIR_avg;
        if (contamination < 0) contamination = 0;
        irDataPoints.push_back({(float)brightness, contamination});
    }

    setLedBrightness(255);

    float sum_x = 0, sum_y = 0, sum_xy = 0, sum_x_squared = 0;
    int n = irDataPoints.size();
    for (const auto& p : irDataPoints) {
        sum_x += p.first;
        sum_y += p.second;
        sum_xy += p.first * p.second;
        sum_x_squared += p.first * p.first;
    }

    float denominator = n * sum_x_squared - sum_x * sum_x;
    if (abs(denominator) < 1e-6) return false;

    float slope = (n * sum_xy - sum_x * sum_y) / denominator;
    float intercept = (sum_y - slope * sum_x) / n;

    float baseCompensation = intercept / 65535.0f;
    float brightnessResponse = slope / 65535.0f;

    configureLEDIRCompensation(baseCompensation, brightnessResponse, true);
    setChannelIRLeakage(baseCompensation * 0.4f, baseCompensation * 0.2f, baseCompensation);

    return true;
}
