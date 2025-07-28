/*!
 * @file sensor_settings.h
 * @brief Easy-to-Adjust Color Sensor Settings
 * @author ESP32-S3 Color Sensor Project
 * @version 2.0
 * @date 2025-01-13
 *
 * =============================================================================
 * 🎛️  COLOR SENSOR CONFIGURATION - EASY ADJUSTMENT PANEL
 * =============================================================================
 *
 * This file contains ALL adjustable settings for the color sensor.
 * Modify values here to tune performance without digging through code.
 *
 * 📍 QUICK SETTINGS GUIDE:
 * - WiFi: Set your network credentials
 * - Color Detection: Adjust distance and calibration
 * - Performance: Tune memory and processing limits
 * - Debug: Control logging and output detail
 *
 * 📋 CURRENT DEFAULTS REFERENCE:
 * All settings show (default: value) - these are the original main.cpp values
 * Change any setting by modifying the #define value on the left
 * Example: #define WIFI_SSID "YourNetwork"     // (default: "Wifi 6")
 */

#ifndef SENSOR_SETTINGS_H
#define SENSOR_SETTINGS_H

// =============================================================================
// 🌐 NETWORK SETTINGS
// =============================================================================

// WiFi Configuration
#define WIFI_SSID "Wifi 6"            // 📶 Your WiFi network name (default: "Wifi 6")
#define WIFI_PASSWORD "Scrofani1985"  // 🔐 Your WiFi password (default: "Scrofani1985")
#define WIFI_TIMEOUT_SECONDS 30       // ⏱️ WiFi connection timeout (default: 30)
#define WEB_SERVER_PORT 80            // 🌍 Web server port (default: 80)

// Access Point Mode (when WiFi not available)
#define AP_SSID "color matcher"         // 📡 AP network name (default: "color matcher")
#define AP_PASSWORD "Scrofani1985"  // 🔐 AP password (default: "Scrofani1985")

// Static IP Configuration
#define STATIC_IP "192.168.0.152"    // 🌐 Static IP address (default: 192.168.0.152)
#define GATEWAY_IP "192.168.0.1"     // 🚪 Gateway address (default: 192.168.0.1)
#define SUBNET_MASK "255.255.255.0"  // 🔗 Subnet mask (default: 255.255.255.0)

// NTP Time Configuration (Melbourne, Australia)
#define NTP_SERVER "pool.ntp.org"           // 🕐 NTP server (default: "pool.ntp.org")
#define NTP_TIMEZONE_OFFSET 39600           // 🌏 Melbourne UTC+11 offset in seconds (39600 = 11*3600)
#define NTP_DAYLIGHT_OFFSET 0               // 🌞 Daylight saving offset (0 = no DST adjustment)

// =============================================================================
// 🎨 COLOR DETECTION SETTINGS
// =============================================================================

// Distance and Measurement Settings - OPTIMIZED FOR MAXIMUM SIGNAL
#define OPTIMAL_SENSOR_DISTANCE_MM 10  // 📏 REDUCED distance for maximum light capture (was 15mm)
#define MAX_SENSOR_DISTANCE_MM 15      // 📏 Maximum recommended distance before signal degrades
#define MIN_SENSOR_DISTANCE_MM 5       // 📏 Minimum distance to avoid LED saturation
#define COLOR_READING_SAMPLES 7        // 🔄 Number of readings to average - increased for stability
#define COLOR_STABILITY_THRESHOLD \
  5                            // 📊 RGB change threshold for stable reading - tighter tolerance
#define SENSOR_SAMPLE_DELAY 3  // ⏲️ Delay between samples in ms - slightly increased for stability

// Physical Setup Requirements
#define AMBIENT_LIGHT_WARNING_THRESHOLD 1000  // 🌞 Warn if ambient light detected above this level
#define SIGNAL_QUALITY_MINIMUM 25000          // 📊 Minimum signal for reliable color measurement

// Sensor Hardware Settings
/*
 * 📊 TCS3430 INTEGRATION TIME REFERENCE CHART
 * Integration Time = (ATIME + 1) × 2.78ms
 * ============================================================================
 * | ATIME  | Cycles | Integration Time | Max ALS Value | Use Case           |
 * |--------|--------|------------------|---------------|--------------------|
 * | 0x00   | 1      | 2.78ms          | 1,023         | Ultra-fast sampling|
 * | 0x01   | 2      | 5.56ms          | 2,047         | Fast motion detect |
 * | 0x10   | 17     | 47.22ms         | 17,407        | Quick measurements |
 * | 0x11   | 18     | 50ms            | 18,431        | Standard fast      |
 * | 0x23   | 36     | 100ms           | 36,863        | ⭐ CURRENT-Balanced|
 * | 0x40   | 65     | 181ms           | 65,535        | High precision     |
 * | 0x80   | 129    | 359ms           | 65,535        | Very precise       |
 * | 0xC0   | 193    | 537ms           | 65,535        | Maximum precision  |
 * | 0xFF   | 256    | 712ms           | 65,535        | Ultra-precise/slow |
 * ============================================================================
 * 💡 For yellow distance issues, try 0x40 (181ms) for better accuracy
 * 🏃 For faster sampling, try 0x11 (50ms)
 * 🎯 For maximum precision, try 0x80 (359ms)
 */
#define SENSOR_INTEGRATION_TIME 0x20  // ⚙️ TCS3430 integration time - 90ms reduced for white calibration
#define SENSOR_SATURATION_THRESHOLD 60000  // 🚨 Saturation detection limit - lowered for better detection

// Intelligent Auto-Adjustment Constants - OPTIMIZED FOR MAXIMUM DYNAMIC RANGE
#define SATURATION_THRESHOLD 62000        // 🚨 Higher threshold for maximum signal utilization
#define OPTIMAL_TARGET_VALUE 45000        // 🎯 INCREASED target for better signal-to-noise ratio
#define OPTIMAL_WINDOW_HIGH 62000         // 62000 - Push closer to saturation for maximum dynamic range
#define OPTIMAL_WINDOW_LOW  20000         // 20000 - Higher minimum for better signal quality
#define VIVID_WHITE_TARGET_SIGNAL 55000   // 🎯 Target signal level for Vivid White calibration

// Gamma Correction Settings for Sensor Linearization
#define SENSOR_GAMMA_CORRECTION 2.2f      // 🔬 Gamma value for sensor linearization (typical: 2.2)
#define ENABLE_GAMMA_CORRECTION true      // 🔬 Enable gamma correction in color pipeline
#define GAMMA_CORRECTION_THRESHOLD 0.04045f // 🔬 Threshold for linear vs gamma correction

#define LED_PIN 5                          // 💡 LED pin number (default: 5)
#define LED_BRIGHTNESS 180  // 🔆 LED brightness 0-255 - INCREASED for better signal-to-noise ratio
#define LED_MAX_BRIGHTNESS 220  // 🔆 Maximum allowed LED brightness for safety
#define LED_MIN_BRIGHTNESS 20   // 🔆 Minimum LED brightness for stable operation

// Color Calibration Fine-Tuning
#define IR_COMPENSATION_FACTOR_1 \
  0.20  // 🔧 IR1 compensation multiplier - reduced for better grey accuracy
#define IR_COMPENSATION_FACTOR_2 \
  0.20  // 🔧 IR2 compensation multiplier - reduced for better grey accuracy
#define RGB_SATURATION_LIMIT \
  255  // 🌈 Restored for higher RGB values - prevents overflow (default: 255)

// Calibration Parameters (from main.cpp defaults)
#define CALIBRATION_IR_COMPENSATION 0.32f  // 🔬 IR compensation factor (default: 0.32)
#define CALIBRATION_R_SLOPE 0.01352f       // 📈 Red channel slope (default: 0.01352)
#define CALIBRATION_R_OFFSET 59.18f        // 📊 Red channel offset (default: 59.18)
#define CALIBRATION_G_SLOPE 0.01535f       // 📈 Green channel slope (default: 0.01535)
#define CALIBRATION_G_OFFSET 34.92f        // 📊 Green channel offset (default: 34.92)
#define CALIBRATION_B_SLOPE 0.02065f       // 📈 Blue channel slope (default: 0.02065)
#define CALIBRATION_B_OFFSET 85.94f        // 📊 Blue channel offset (default: 85.94)

// Yellow Detection Optimization (based on your distance findings)
#define YELLOW_DISTANCE_COMPENSATION true  // 🟡 Enable distance-based yellow tuning (default: true)
#define YELLOW_MIN_RATIO 0.85              // 🟡 Minimum R+G vs B ratio for yellow (default: 0.85)
#define YELLOW_BRIGHTNESS_THRESHOLD \
  200  // 🟡 Minimum brightness for yellow detection (default: 200)

// =============================================================================
// 🧠 KD-TREE PERFORMANCE SETTINGS
// =============================================================================

// Memory Management
#define ENABLE_KDTREE 0               // 🌳 Enable KD-tree 1=ON, 0=OFF (default: 1) - TEMPORARILY DISABLED DUE TO BUG
#define KDTREE_MAX_COLORS 4500        // 📊 Maximum colors in KD-tree (default: 4500)
#define PSRAM_SAFETY_MARGIN_KB 2048   // 💾 PSRAM to keep free in KB (default: 2048)
#define KDTREE_LOAD_TIMEOUT_MS 20000  // ⏱️ KD-tree build timeout in ms (default: 20000)

// Search Performance
#define KDTREE_SEARCH_TIMEOUT_MS 50    // 🔍 KD-tree search timeout in ms (default: 50)
#define BINARY_SEARCH_TIMEOUT_MS 5000  // 🔍 Binary search timeout in ms (default: 5000)
#define WIFI_TIMEOUT_MS 30000          // 📶 WiFi connection timeout in ms (default: 30000)
#define COLOR_LOAD_TIMEOUT_SECONDS 20  // 📂 Color database load timeout (default: 20)

// Progress Reporting
#define PROGRESS_REPORT_INTERVAL 500   // 📈 Report every N colors loaded (default: 500)
#define MEMORY_CHECK_INTERVAL_MS 1000  // 💾 Memory status check interval (default: 1000)

// =============================================================================
// 🐛 DEBUG AND LOGGING SETTINGS
// =============================================================================

// Default Log Level
#define DEFAULT_LOG_LEVEL LOG_INFO  // 📝 Default logging level (default: LOG_INFO)

// Log Levels (set to false to disable specific logging)
#define DEBUG_SENSOR_READINGS true     // 🔬 Show detailed XYZ→RGB conversion (default: true)
#define DEBUG_COLOR_MATCHING true      // 🎨 Show color search results (default: true)
#define DEBUG_MEMORY_USAGE true        // 💾 Show memory statistics (default: true)
#define DEBUG_PERFORMANCE_TIMING true  // ⏱️ Show operation timing (default: true)
#define DEBUG_WIFI_DETAILS false       // 📶 Show WiFi connection details (default: false)

// Output Format
#define SENSOR_READING_INTERVAL_MS 2000  // 📊 How often to output color readings (default: 2000)
#define COMPACT_OUTPUT_MODE false        // 📝 Use compact vs detailed output (default: false)
#define SHOW_RAW_XYZ_VALUES true         // 🔢 Include raw XYZ in output (default: true)

// =============================================================================
// ⚡ ADVANCED PERFORMANCE TUNING
// =============================================================================

// TCS3430 Sensor Configuration
#define TCS3430_INTEGRATION_TIME_MS 50   // 📡 Sensor integration time - reduced for white calibration
#define TCS3430_GAIN_SETTING 4           // 📡 Sensor gain 1,4,16,64 - reduced for white calibration
#define TCS3430_AUTO_GAIN_ADJUST true    // 📡 Enable automatic gain adjustment (default: true)

// System Performance
#define WATCHDOG_TIMEOUT_SECONDS 10  // 🐕 System watchdog timeout (default: 10)
#define TASK_YIELD_INTERVAL_MS 10    // 🔄 How often to yield to other tasks (default: 10)
#define LOW_MEMORY_WARNING_KB 100    // ⚠️ Warning when free memory below this (default: 100)

// Color Database Optimization
#define PRELOAD_COMMON_COLORS true    // 🎨 Preload frequently matched colors (default: true)
#define COLOR_CACHE_SIZE 50           // 💾 Number of colors to cache (default: 50)
#define ENABLE_COLOR_PREDICTION true  // 🔮 Predict next likely color matches

// =============================================================================
// 🎛️ QUICK TUNING PRESETS
// =============================================================================

// Uncomment ONE preset below for quick configuration:

// 🏃 SPEED PRESET - Prioritize fast response
// #define PRESET_SPEED
#ifdef PRESET_SPEED
  #undef COLOR_READING_SAMPLES
  #define COLOR_READING_SAMPLES 3
  #undef KDTREE_SEARCH_TIMEOUT_MS
  #define KDTREE_SEARCH_TIMEOUT_MS 25
  #undef DEBUG_SENSOR_READINGS
  #define DEBUG_SENSOR_READINGS false
#endif

// 🎯 ACCURACY PRESET - Prioritize color accuracy
// #define PRESET_ACCURACY
#ifdef PRESET_ACCURACY
  #undef COLOR_READING_SAMPLES
  #define COLOR_READING_SAMPLES 10
  #undef COLOR_STABILITY_THRESHOLD
  #define COLOR_STABILITY_THRESHOLD 5
  #undef TCS3430_INTEGRATION_TIME_MS
  #define TCS3430_INTEGRATION_TIME_MS 200
#endif

// 🔋 POWER_SAVING PRESET - Minimize power consumption
// #define PRESET_POWER_SAVING
#ifdef PRESET_POWER_SAVING
  #undef SENSOR_READING_INTERVAL_MS
  #define SENSOR_READING_INTERVAL_MS 5000
  #undef DEBUG_SENSOR_READINGS
  #define DEBUG_SENSOR_READINGS false
  #undef ENABLE_KDTREE
  #define ENABLE_KDTREE 0
#endif

// =============================================================================
// 🔧 VALIDATION AND SAFETY CHECKS
// =============================================================================

// Compile-time validation
#if COLOR_READING_SAMPLES < 1
  #error "COLOR_READING_SAMPLES must be at least 1"
#endif

#if PSRAM_SAFETY_MARGIN_KB < 500
  #warning "PSRAM_SAFETY_MARGIN_KB is very low, consider increasing"
#endif

#if TCS3430_INTEGRATION_TIME_MS < 50
  #warning "Very short integration time may reduce accuracy"
#endif

// =============================================================================
// 📋 SETTINGS SUMMARY DISPLAY
// =============================================================================

// Function to display current settings (call during startup from main.cpp)
// Note: Serial must be initialized before calling this function
void displayCurrentSettings();  // Declaration only - defined in main.cpp

#endif  // SENSOR_SETTINGS_H
