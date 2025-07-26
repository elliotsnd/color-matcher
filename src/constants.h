#ifndef CONSTANTS_H
#define CONSTANTS_H

// RGB Constants
#define RGB_MAX_VALUE 255
#define RGB_DIVISOR 256  // For 16-bit to 8-bit conversion

// IR Compensation Factors - REMOVED TO PREVENT REDEFINITION
// These are now defined exclusively in sensor_settings.h

// Sensor Range Constants
#define SENSOR_MAX_VALUE 65535
#define SENSOR_MIN_OPTIMAL_RANGE 5000
#define SENSOR_MAX_OPTIMAL_RANGE 45000

// Battery Voltage Constants
#define BATTERY_FULL_VOLTAGE 4.0F
#define BATTERY_HIGH_VOLTAGE 3.7F
#define BATTERY_MEDIUM_VOLTAGE 3.4F
#define BATTERY_LOW_VOLTAGE 2.5F
#define BATTERY_ROUND_FACTOR 0.5F

// Timing Constants
#define WIFI_CONNECT_TIMEOUT_MS 10000
#define DELAY_MS_500 500
#define DELAY_MS_1000 1000

// Test Score Constants
#define TEST_SCORE_MAX 100

// Calibration Constants
#define CALIBRATION_SAMPLES 7  // Number of samples for calibration readings
#define TEST_SCORE_GOOD 80
#define TEST_SCORE_MEDIUM 60
#define TEST_SCORE_LOW 40
#define TEST_SCORE_POOR 20
#define TEST_SCORE_VERY_POOR 10

// Vivid White Target RGB Values
#define VIVID_WHITE_TARGET_R 247
#define VIVID_WHITE_TARGET_G 248
#define VIVID_WHITE_TARGET_B 244

// Auto-gain Constants
#define AUTO_GAIN_TARGET_VALUE 800
#define AUTO_GAIN_TARGET_INTEGRATION_TIME 250.0F
#define IR_INTERFERENCE_THRESHOLD 25.0F

// Sample Count Constants
#define DEFAULT_SAMPLE_COUNT 10
#define OPTIMIZED_SAMPLE_COUNT 15
#define OPTIMIZED_SAMPLE_DELAY 5

// Memory Constants
#define MEMORY_BYTES_PER_KB 1024

// Comment for color science parameters
// X, Y, Z are standard CIE color space coordinates
// R, G, B are standard RGB color space coordinates
// These short parameter names are industry standard in color science

// Network Constants
#define AP_IP "192.168.4.1"  // Default AP IP address

// HTTP Status Constants
#define HTTP_STATUS_OK 200
#define HTTP_STATUS_INTERNAL_SERVER_ERROR 500

// JSON Constants
#define JSON_FIELD_STATUS "status"
#define JSON_FIELD_MESSAGE "message"
#define JSON_STATUS_SUCCESS "success"
// JSON_CONTENT_TYPE moved to main.cpp as constexpr (modern C++)

// Timing Constants
#define TIMING_WIFI_RETRY_DELAY_MS 500
#define TIMING_STABILIZATION_DELAY_MS 1000
#define TIMING_OPTIMIZATION_INTERVAL_MS 5000
#define TIMING_STATUS_LOG_INTERVAL_MS 10000
#define TEST_DELAY_MS 100

// Sensor Constants
#define SENSOR_MAX_SAMPLES 20
#define COLOR_RGB_MAX 255

#endif  // CONSTANTS_H