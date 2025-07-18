﻿/*!
 * @file getXYZIRData.ino
 * @brief Definitive, Fully Calibrated Code with Web Server
 * @copyright  Copyright (c) 2010 DFRobot Co.Ltd (http://www.dfrobot.com)
 * @licence     The MIT License (MIT)
 * @author [yangfeng]<feng.yang@dfrobot.com>
 * @version  V1.0
 * @date  2021-01-26
 * @get from https://www.dfrobot.com
 * @url  https://github.com/DFRobot/DFRobot_TCS3430
 */

// ðŸŽ›ï¸ EASY SETTINGS - All adjustable parameters are in sensor_settings.h
#include "sensor_settings.h"

#include <DFRobot_TCS3430.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <Wire.h>
#include <esp_heap_caps.h>
#include "dulux_simple_reader.h"
#include "CIEDE2000.h"
// Use lightweight KD-tree optimized for embedded systems
#if ENABLE_KDTREE
#include "lightweight_kdtree.h"
#endif
#include <UMS3.h>

// Battery monitoring for ProS3 - using official UMS3 library
// ProS3 has built-in battery monitoring circuit and I2C fuel gauge
UMS3 ums3;

// Custom PSRAM allocator for ArduinoJson v7
class PsramAllocator : public ArduinoJson::Allocator {
public:
    void* allocate(size_t size) override {
        return heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    }

    void deallocate(void* pointer) override {
        heap_caps_free(pointer);
    }

    void* reallocate(void* ptr, size_t new_size) override {
        return heap_caps_realloc(ptr, new_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    }
};

// Logging system
enum LogLevel {
  LOG_ERROR = 0,
  LOG_WARN = 1,
  LOG_INFO = 2,
  LOG_DEBUG = 3
};

class Logger {
private:
  static LogLevel currentLevel;
  
public:
  static void setLevel(LogLevel level) {
    currentLevel = level;
  }
  
  static void error(const String& message) {
    if (currentLevel >= LOG_ERROR) {
      Serial.print("[ERROR] ");
      Serial.println(message);
    }
  }
  
  static void warn(const String& message) {
    if (currentLevel >= LOG_WARN) {
      Serial.print("[WARN] ");
      Serial.println(message);
    }
  }
  
  static void info(const String& message) {
    if (currentLevel >= LOG_INFO) {
      Serial.print("[INFO] ");
      Serial.println(message);
    }
  }
  
  static void debug(const String& message) {
    if (currentLevel >= LOG_DEBUG) {
      Serial.print("[DEBUG] ");
      Serial.println(message);
    }
  }
  
  // Convenience methods for formatted output
  static void info(const String& prefix, int value) {
    if (currentLevel >= LOG_INFO) {
      Serial.print("[INFO] ");
      Serial.print(prefix);
      Serial.println(value);
    }
  }
  
  static void debug(const String& prefix, int value) {
    if (currentLevel >= LOG_DEBUG) {
      Serial.print("[DEBUG] ");
      Serial.print(prefix);
      Serial.println(value);
    }
  }
};

// Initialize static member
LogLevel Logger::currentLevel = DEFAULT_LOG_LEVEL; // Configurable default log level

// WiFi credentials
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

// AP mode credentials
const char* ap_ssid = AP_SSID;
const char* ap_password = AP_PASSWORD;

// Static IP configuration
IPAddress local_IP, gateway, subnet;
void initializeIPAddresses() {
  local_IP.fromString(STATIC_IP);
  gateway.fromString(GATEWAY_IP);
  subnet.fromString(SUBNET_MASK);
}

// Web server on port 80
AsyncWebServer server(80);

// Function declarations
void handleGetSettings(AsyncWebServerRequest *request);
void handleSetLedBrightness(AsyncWebServerRequest *request);
void handleSetIntegrationTime(AsyncWebServerRequest *request);
void handleSetIRCompensation(AsyncWebServerRequest *request);
void handleSetColorSamples(AsyncWebServerRequest *request);
void handleSetSampleDelay(AsyncWebServerRequest *request);
void handleSetDebugSettings(AsyncWebServerRequest *request);

// Fast live feed and color name API handlers
void handleFastColorAPI(AsyncWebServerRequest *request);
void handleColorNameAPI(AsyncWebServerRequest *request);
void handleForceColorLookup(AsyncWebServerRequest *request);

// Calibration API handlers
void handleGetCalibration(AsyncWebServerRequest *request);
void handleTuneVividWhite(AsyncWebServerRequest *request);
void convertXYZtoRGB_Uncalibrated(uint16_t X, uint16_t Y, uint16_t Z, uint8_t &R, uint8_t &G, uint8_t &B);
void convertXYZtoRGB_Calibrated(uint16_t X, uint16_t Y, uint16_t Z, uint16_t IR1, uint16_t IR2, uint8_t &R, uint8_t &G, uint8_t &B);

// Battery voltage monitoring
float getBatteryVoltage();
// Detect if VBUS (USB power) is present
bool getVbusPresent();
void handleBatteryAPI(AsyncWebServerRequest *request);

DFRobot_TCS3430 TCS3430;

// I2C pin definitions for ESP32-S3 ProS3
#define SDA_PIN 3
#define SCL_PIN 4

// Battery monitoring for ProS3 - using official UMSeriesD library
// The library handles GPIO pins and voltage dividers automatically

// Set the optimized LED pin
int LEDpin = LED_PIN;

const uint16_t SATURATION_THRESHOLD = SENSOR_SATURATION_THRESHOLD;

// === START OF FINAL, DEFINITIVE CALIBRATION PARAMETERS ===
// Confirmed to produce accurate results for three targets.

// Runtime Settings Structure - Can be modified via web interface
struct RuntimeSettings {
  // Color Detection Settings
  int colorReadingSamples = COLOR_READING_SAMPLES;
  int colorStabilityThreshold = COLOR_STABILITY_THRESHOLD;
  int sensorSampleDelay = SENSOR_SAMPLE_DELAY;
  int optimalSensorDistance = OPTIMAL_SENSOR_DISTANCE_MM;
  
  // Sensor Hardware Settings
  uint8_t sensorIntegrationTime = SENSOR_INTEGRATION_TIME;
  uint16_t sensorSaturationThreshold = SENSOR_SATURATION_THRESHOLD;
  int ledBrightness = LED_BRIGHTNESS;
  
  // Calibration Mode Selection
  bool useDFRobotLibraryCalibration = false;  // false = custom quadratic, true = DFRobot library matrix
  
  // Color Calibration Settings
  float irCompensationFactor1 = IR_COMPENSATION_FACTOR_1;
  float irCompensationFactor2 = IR_COMPENSATION_FACTOR_2;
  uint8_t rgbSaturationLimit = RGB_SATURATION_LIMIT;
  
  // Matrix-based calibration (wide-range)
  bool useMatrixCalibration = true; // true = use 3Ã—3 matrix+offset instead of per-channel quadratic
  
  // Bright-range matrix (used when Y > dynamicThreshold) - Optimized for vivid white RGB(247,248,244)
  float brightMatrix[9] = { 0.0280f, -0.0045f, -0.0070f,
                            -0.0055f,  0.0254f,  0.0005f,
                             0.0022f, -0.0042f,  0.0545f };
  float brightOffset[3] = { 0.0f, 0.0f, 0.0f };
  
  // Dark-range matrix (used when Y <= dynamicThreshold) - Fine-tuned for precise RGB(168,160,147)
  float darkMatrix[9]   = { 0.0291f, -0.0032f, -0.0024f,
                           -0.0040f,  0.0268f,   0.0016f,
                            0.0016f, -0.0030f,   0.0606f };
  float darkOffset[3]   = { 0.0f, 0.0f, 0.0f };
  
  // Dynamic calibration control
  bool enableDynamicCalibration = true;
  float dynamicThreshold = 8000.0f; // Y threshold for bright/dark matrix switching - restored for proper grey detection
  
  // Auto-adjust integration
  bool enableAutoAdjust = true;
  float autoSatHigh = 0.9f;
  float autoSatLow = 0.1f;
  uint8_t minIntegrationTime = 0x10;
  uint8_t maxIntegrationTime = 0x80;
  uint8_t integrationStep = 0x10;
  
  // Quadratic calibration (legacy/backup)
  float redA = 5.756615248518086e-06f;
  float redB = -0.10824971353127427f;
  float redC = 663.2283515839658f;
  float greenA = 7.700364703908128e-06f;
  float greenB = -0.14873455804115546f;
  float greenC = 855.288778468652f;
  float blueA = -2.7588632792769936e-06f;
  float blueB = 0.04959423885676833f;
  float blueC = 35.55576869603341f;
  
  // Legacy Calibration Parameters (for compatibility)
  float irCompensation = CALIBRATION_IR_COMPENSATION;
  float rSlope = CALIBRATION_R_SLOPE;
  float rOffset = CALIBRATION_R_OFFSET;
  float gSlope = CALIBRATION_G_SLOPE;
  float gOffset = CALIBRATION_G_OFFSET;
  float bSlope = CALIBRATION_B_SLOPE;
  float bOffset = CALIBRATION_B_OFFSET;
  
  // Yellow Detection Settings
  bool yellowDistanceCompensation = YELLOW_DISTANCE_COMPENSATION;
  float yellowMinRatio = YELLOW_MIN_RATIO;
  int yellowBrightnessThreshold = YELLOW_BRIGHTNESS_THRESHOLD;
  
  // Performance Settings
  int kdtreeMaxColors = KDTREE_MAX_COLORS;
  int kdtreeSearchTimeout = KDTREE_SEARCH_TIMEOUT_MS;
  int binarySearchTimeout = BINARY_SEARCH_TIMEOUT_MS;
  bool enableKdtree = ENABLE_KDTREE;
  
  // Debug Settings
  bool debugSensorReadings = DEBUG_SENSOR_READINGS;
  bool debugColorMatching = DEBUG_COLOR_MATCHING;
  bool debugMemoryUsage = DEBUG_MEMORY_USAGE;
  bool debugPerformanceTiming = DEBUG_PERFORMANCE_TIMING;
  int sensorReadingInterval = SENSOR_READING_INTERVAL_MS;
  
  // White Optimized Coefficients
  float whiteRedA = 0.000001f;
  float whiteRedB = -0.01278f;
  float whiteRedC = 280.0f;
  float whiteGreenA = 0.000002f;
  float whiteGreenB = -0.031f;
  float whiteGreenC = 360.0f;
  float whiteBlueA = 0.0000008f;
  float whiteBlueB = 0.019f;
  float whiteBlueC = 160.0f;
  // Grey Optimized Coefficients - tuned for target RGB(168,160,147)
  float greyRedA = 0.0000007f;
  float greyRedB = -0.012f;
  float greyRedC = 168.0f;
  float greyGreenA = 0.0000011f;
  float greyGreenB = -0.015f;
  float greyGreenC = 160.0f;
  float greyBlueA = 0.0000009f;
  float greyBlueB = -0.008f;
  float greyBlueC = 147.0f;
  
  // Dynamic IR compensation - adjusted for grey port accuracy
  bool enableDynamicIR = true;
  float irHighThreshold = 400.0f;  // Lower threshold for grey tones
  float irLowFactor = 0.15f;       // Reduced for grey accuracy
  float irHighFactor = 0.10f;      // Reduced for grey accuracy
};

// Global runtime settings instance
RuntimeSettings settings;



// === END OF CALIBRATION PARAMETERS ===

// Simple binary color database reader
// Color database instances
DuluxSimpleReader simpleColorDB;
#if ENABLE_KDTREE
LightweightKDTree kdTreeColorDB;
#endif

// Legacy compatibility structure (for fallback colors only)
struct DuluxColor {
  String name;      // Color name
  String code;      // Color code
  uint8_t r, g, b;  // RGB values
  String lrv;       // Light Reflectance Value
  String id;        // Unique ID
};

// Fallback color database (only used if binary loading fails)
DuluxColor* fallbackColorDatabase = nullptr;
int fallbackColorCount = 0;

// Hardcoded fallback colors for emergency use
const DuluxColor fallbackColors[] PROGMEM = {
  {"Pure Brilliant White", "10BB83", 255, 255, 255, "89", "1"},
  {"Natural White", "10BB31", 252, 251, 247, "85", "2"},
  {"Antique White", "20YY83", 248, 243, 234, "82", "3"},
  {"Light Grey", "00NN79", 200, 200, 200, "65", "4"},
  {"Medium Grey", "00NN53", 135, 135, 135, "35", "5"},
  {"Charcoal", "00NN21", 54, 54, 54, "8", "6"},
  {"Pure Black", "00NN05", 13, 13, 13, "2", "7"},
  {"Bright Red", "10YR68", 218, 59, 59, "25", "8"},
  {"Forest Green", "30GY25", 34, 102, 34, "15", "9"},
  {"Sky Blue", "70BG65", 135, 206, 235, "55", "10"}
};

// Load fallback colors when main database fails
bool loadFallbackColors() {
  const int fallbackCount = sizeof(fallbackColors) / sizeof(fallbackColors[0]);
  Logger::warn("Loading fallback color database with " + String(fallbackCount) + " colors");

  // Allocate memory for fallback colors in PSRAM if available
  fallbackColorDatabase = (DuluxColor*)heap_caps_malloc(sizeof(DuluxColor) * fallbackCount, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!fallbackColorDatabase) {
    Logger::warn("Failed to allocate fallback colors in PSRAM, trying regular heap");
    fallbackColorDatabase = new DuluxColor[fallbackCount];
    if (!fallbackColorDatabase) {
      Logger::error("Failed to allocate memory for fallback colors");
      return false;
    }
  }

  // Copy fallback colors from PROGMEM to RAM
  for (int i = 0; i < fallbackCount; i++) {
    fallbackColorDatabase[i].name = String(fallbackColors[i].name);
    fallbackColorDatabase[i].code = String(fallbackColors[i].code);
    fallbackColorDatabase[i].r = fallbackColors[i].r;
    fallbackColorDatabase[i].g = fallbackColors[i].g;
    fallbackColorDatabase[i].b = fallbackColors[i].b;
    fallbackColorDatabase[i].lrv = String(fallbackColors[i].lrv);
    fallbackColorDatabase[i].id = String(fallbackColors[i].id);
  }

  fallbackColorCount = fallbackCount;

  Logger::info("Fallback color database loaded successfully with " + String(fallbackColorCount) + " colors");
  return true;
}

// Load color database from binary file with optimized memory usage
bool loadColorDatabase() {
  Logger::info("=== Starting binary color database load process ===");
  unsigned long startTime = millis(); // Track load time

  // Check available memory before starting
  size_t freeHeap = esp_get_free_heap_size();
  size_t freePsram = psramFound() ? ESP.getFreePsram() : 0;
  
  Logger::info("Free heap before loading: " + String(freeHeap / 1024) + " KB");
  if (psramFound()) {
    Logger::info("Free PSRAM before loading: " + String(freePsram / 1024) + " KB");
    
    // Performance optimization: Check if we have enough PSRAM for optimal performance
    if (freePsram < (PSRAM_SAFETY_MARGIN_KB * 1024)) {
      Logger::warn("Low PSRAM detected (" + String(freePsram / 1024) + " KB < " + String(PSRAM_SAFETY_MARGIN_KB) + " KB safety margin)");
      Logger::warn("KD-tree will be disabled to conserve memory");
      settings.enableKdtree = false;  // Disable KD-tree for low memory situations
    }
  } else {
    Logger::error("PSRAM not available - using fallback color database");
    settings.enableKdtree = false;  // Disable KD-tree without PSRAM
    return loadFallbackColors();
  }

  Logger::info("Memory check complete, proceeding with binary file loading...");

  // Try to open binary database first (preferred method)
  Logger::info("Attempting to open binary color database: /dulux.bin");
  if (simpleColorDB.openDatabase("/dulux.bin")) {
    unsigned long loadTime = millis() - startTime;
    size_t colorCount = simpleColorDB.getColorCount();
    
    Logger::info("Binary color database opened successfully!");
    Logger::info("Colors available: " + String(colorCount));
    Logger::info("Open time: " + String(loadTime) + "ms");
    Logger::info("PSRAM free after open: " + String(ESP.getFreePsram() / 1024) + " KB");
    
    // Performance optimization: Conditionally enable KD-tree based on database size
    bool shouldUseKdtree = settings.enableKdtree;  // Start with user/memory setting
    
    if (shouldUseKdtree && colorCount <= 1000) {
      Logger::info("Small database detected (" + String(colorCount) + " colors â‰¤ 1000)");
      Logger::info("KD-tree overhead not justified - using direct binary search for optimal performance");
      shouldUseKdtree = false;
    } else if (shouldUseKdtree && colorCount > 1000) {
      Logger::info("Large database detected (" + String(colorCount) + " colors > 1000)");
      Logger::info("KD-tree will provide significant search speed improvements");
    }
    
    // Update runtime setting based on optimization analysis
    settings.enableKdtree = shouldUseKdtree;
    
#if ENABLE_KDTREE
    if (shouldUseKdtree) {
    // Initialize KD-tree with data from binary database
    Logger::info("Building lightweight KD-tree for optimized color search...");
    unsigned long kdStartTime = millis();
    
    Logger::info("Loading " + String(colorCount) + " colors into KD-tree...");
    
    // Performance monitoring: Check available memory before KD-tree construction
    size_t heapBeforeKd = ESP.getFreeHeap();
    size_t psramBeforeKd = ESP.getFreePsram();
    Logger::info("Memory before KD-tree: Heap=" + String(heapBeforeKd / 1024) + " KB, PSRAM=" + String(psramBeforeKd / 1024) + " KB");
    
    // Safety check for large datasets
    if (colorCount > 10000) {
      Logger::warn("Very large color dataset detected (" + String(colorCount) + " colors)");
      Logger::warn("This may take significant time and memory - consider reducing KDTREE_MAX_COLORS");
    }
    
    // Limit colors to KDTREE_MAX_COLORS setting for memory management
    size_t effectiveColorCount = min(colorCount, (size_t)settings.kdtreeMaxColors);
    if (effectiveColorCount < colorCount) {
      Logger::warn("Limiting KD-tree to " + String(effectiveColorCount) + " colors (from " + String(colorCount) + ") due to KDTREE_MAX_COLORS setting");
    }
    
    // Create vector of color points for the lightweight KD-tree
    PSRAMColorVector colorPoints;
    colorPoints.reserve(effectiveColorCount);
    
    // Load colors with progress logging and timeout protection
    size_t loadedCount = 0;
    unsigned long loadStartTime = millis();
    const unsigned long maxLoadTime = KDTREE_LOAD_TIMEOUT_MS; // Configurable timeout for loading
    
    for (size_t i = 0; i < effectiveColorCount; i++) {
      // Check for timeout during loading
      if (millis() - loadStartTime > maxLoadTime) {
        Logger::warn("Color loading timeout after " + String((millis() - loadStartTime) / 1000) + " seconds");
        Logger::warn("Loaded " + String(loadedCount) + " of " + String(effectiveColorCount) + " colors before timeout");
        break;
      }
      
      SimpleColor color;
      if (simpleColorDB.getColorByIndex(i, color)) {
        ColorPoint point(color.r, color.g, color.b, (uint16_t)i);
        colorPoints.push_back(point);
        loadedCount++;
        
        // Progress logging for large datasets with memory monitoring
        if (effectiveColorCount > 1000 && (i % 500 == 0 || i == effectiveColorCount - 1)) {
          Logger::info("Loaded " + String(i + 1) + "/" + String(effectiveColorCount) + " colors");
          
          // Performance monitoring: Check memory usage during loading
          size_t currentFreeHeap = ESP.getFreeHeap();
          size_t currentFreePsram = ESP.getFreePsram();
          unsigned long elapsedTime = millis() - loadStartTime;
          
          Logger::info("Memory: Heap=" + String(currentFreeHeap / 1024) + " KB, PSRAM=" + String(currentFreePsram / 1024) + " KB, Time=" + String(elapsedTime) + "ms");
          
          // Performance optimization: Monitor memory usage and abort if critical
          if (currentFreeHeap < 50000) {
            Logger::error("Critical heap memory low (" + String(currentFreeHeap) + " bytes) - stopping KD-tree construction");
            Logger::error("Consider reducing KDTREE_MAX_COLORS or increasing PSRAM_SAFETY_MARGIN_KB");
            break;
          }
          
          if (currentFreePsram < (PSRAM_SAFETY_MARGIN_KB * 1024 / 2)) {
            Logger::warn("PSRAM approaching safety margin (" + String(currentFreePsram / 1024) + " KB) - may limit performance");
          }
          
          // Yield to watchdog and other tasks
          delay(1);
        }
      } else {
        // Log failed color reads
        if (i % 1000 == 0) {
          Logger::warn("Failed to read color at index " + String(i));
        }
      }
    }
    
    Logger::info("Successfully loaded " + String(loadedCount) + " colors for KD-tree");
    
    if (loadedCount == 0) {
      Logger::error("No colors loaded - skipping KD-tree construction");
      Logger::warn("Falling back to binary database search");
      settings.enableKdtree = false;  // Disable failed KD-tree
    } else {
      Logger::info("Starting lightweight KD-tree construction...");
      
      if (kdTreeColorDB.build(colorPoints)) {
        unsigned long kdLoadTime = millis() - kdStartTime;
        size_t memoryUsage = kdTreeColorDB.getMemoryUsage();
        
        Logger::info("ðŸŽ¯ KD-tree built successfully in " + String(kdLoadTime) + "ms");
        Logger::info("ðŸ“Š KD-tree stats: " + String(kdTreeColorDB.getNodeCount()) + " nodes, " + String(memoryUsage) + " bytes");
        Logger::info("ðŸš€ Search performance: O(log " + String(loadedCount) + ") vs O(" + String(loadedCount) + ") linear");
        Logger::info("ðŸ’¾ PSRAM after KD-tree: " + String(ESP.getFreePsram() / 1024) + " KB free");
        
        // Performance validation: Estimate search speed improvement
        float speedupFactor = (float)loadedCount / log2(loadedCount);
        Logger::info("âš¡ Estimated search speedup: " + String(speedupFactor, 1) + "x faster than linear search");
        
      } else {
        Logger::error("Failed to build KD-tree - falling back to binary database only");
        Logger::warn("This may indicate insufficient memory or corrupted color data");
        settings.enableKdtree = false;  // Disable failed KD-tree
      }
    }
    
    } else {
      Logger::info("KD-tree disabled by optimization logic - using binary database only");
      Logger::info("This provides optimal performance for current configuration");
    }
#else
    Logger::info("KD-tree disabled at compile time - using binary database only");
#endif
    
    return true;
  }

  // Fallback to JSON loading if binary fails
  Logger::warn("Binary database loading failed, trying JSON fallback...");
  Logger::info("Attempting to open /dulux.json...");
  File file = LittleFS.open("/dulux.json", "r");
  if (!file) {
    Logger::error("Failed to open dulux.json file, trying alternative filename...");
    Logger::info("Attempting to open /dulux_colors.json...");
    file = LittleFS.open("/dulux_colors.json", "r");
    if (!file) {
      Logger::error("Failed to open any color database files - using fallback colors");
      return loadFallbackColors();
    }
    Logger::info("Using fallback dulux_colors.json file");
  } else {
    Logger::info("dulux.json file opened successfully");
  }

  size_t fileSize = file.size();
  Logger::debug("JSON file size: " + String(fileSize) + " bytes");

  // Validate file size is reasonable
  if (fileSize == 0) {
    Logger::error("Color database file is empty");
    file.close();
    return loadFallbackColors();
  }

  if (fileSize > 10 * 1024 * 1024) { // 10MB limit
    Logger::error("Color database file is too large: " + String(fileSize) + " bytes");
    file.close();
    return loadFallbackColors();
  }

  Logger::warn("JSON loading is deprecated and uses more memory. Consider using binary format.");
  Logger::info("Converting to binary format would save " + String((fileSize * 83) / 100) + " bytes");

  file.close();
  Logger::error("JSON fallback loading not implemented in binary version - using fallback colors");
  return loadFallbackColors();
}
// Calculate color distance using CIEDE2000 algorithm
float calculateColorDistance(uint8_t r1, uint8_t g1, uint8_t b1, uint8_t r2, uint8_t g2, uint8_t b2) {
  // Convert both colors to LAB colorspace
  CIEDE2000::LAB lab1, lab2;
  rgbToLAB(r1, g1, b1, lab1);
  rgbToLAB(r2, g2, b2, lab2);

  // Calculate CIEDE2000 distance
  double distance = CIEDE2000::CIEDE2000(lab1, lab2);
  return (float)distance;
}

// Find the closest Dulux color match using KD-tree (optimized)
String findClosestDuluxColor(uint8_t r, uint8_t g, uint8_t b) {
  if (settings.debugColorMatching) {
    Logger::debug("Finding closest color for RGB(" + String(r) + "," + String(g) + "," + String(b) + ")");
  }

  unsigned long searchStartTime = micros();  // Performance monitoring
  String searchMethod = "Unknown";
  String result = "Unknown Color";

#if ENABLE_KDTREE
  // Try KD-tree search first (fastest - O(log n) average case) if enabled and built
  if (settings.enableKdtree && kdTreeColorDB.isBuilt()) {
    searchMethod = "KD-Tree";
    ColorPoint closest = kdTreeColorDB.findNearest(r, g, b);
    if (closest.index > 0) {
      // Get the full color data using the index
      SimpleColor color;
      if (simpleColorDB.getColorByIndex(closest.index, color)) {
        result = String(color.name) + " (" + String(color.code) + ")";
        
        if (settings.debugColorMatching) {
          unsigned long searchTime = micros() - searchStartTime;
          Logger::debug("KD-tree search completed in " + String(searchTime) + "Î¼s. Best match: " + result);
        }
        return result;
      }
    }
    Logger::warn("KD-tree search failed, falling back to binary database");
  } else if (settings.enableKdtree && !kdTreeColorDB.isBuilt()) {
    Logger::warn("KD-tree enabled but not built - check initialization");
  }
#endif

  // Fallback to simple binary database with optimized search (O(n) but optimized)
  SimpleColor closestColor;
  if (simpleColorDB.findClosestColor(r, g, b, closestColor)) {
    searchMethod = "Binary DB";
    result = String(closestColor.name) + " (" + String(closestColor.code) + ")";
    
    if (settings.debugColorMatching) {
      unsigned long searchTime = micros() - searchStartTime;
      Logger::debug("Binary search completed in " + String(searchTime) + "Î¼s. Best match: " + result);
    }
    return result;
  }

  // Fallback to legacy database if available (slowest - O(n) unoptimized)
  if (fallbackColorDatabase != nullptr && fallbackColorCount > 0) {
    searchMethod = "Fallback DB";
    Logger::warn("Using slow fallback color database with " + String(fallbackColorCount) + " colors...");

    float minDistance = 999999.0f;
    String bestMatch = "Unknown";
    int bestIndex = -1;

    // Search through fallback color database (SLOW - O(n) operation)
    for (int i = 0; i < fallbackColorCount; i++) {
      float distance = calculateColorDistance(r, g, b,
        fallbackColorDatabase[i].r, fallbackColorDatabase[i].g, fallbackColorDatabase[i].b);

      if (distance < minDistance) {
        minDistance = distance;
        bestIndex = i;
        bestMatch = fallbackColorDatabase[i].name + " (" + fallbackColorDatabase[i].code + ")";

        // Early exit for perfect matches
        if (distance < 0.1f) {
          if (settings.debugColorMatching) {
            Logger::debug("Perfect match found: " + bestMatch);
          }
          break;
        }
      }
    }

    result = bestMatch;
    
    if (settings.debugColorMatching) {
      unsigned long searchTime = micros() - searchStartTime;
      Logger::debug("Fallback search completed in " + String(searchTime) + "Î¼s. Best match: " + result + " (distance: " + String(minDistance) + ")");
    }
    return result;
  }

  // Final fallback to basic color names
  searchMethod = "Basic Classification";
  Logger::warn("No color database available, using basic color classification");
  if (r > 200 && g > 200 && b > 200) result = "Light Color";
  else if (r < 50 && g < 50 && b < 50) result = "Dark Color";
  else if (r > g && r > b) result = "Red Tone";
  else if (g > r && g > b) result = "Green Tone";
  else if (b > r && b > g) result = "Blue Tone";
  else result = "Mixed Color";
  
  if (settings.debugColorMatching) {
    unsigned long searchTime = micros() - searchStartTime;
    Logger::debug(searchMethod + " completed in " + String(searchTime) + "Î¼s. Result: " + result);
  }
  
  return result;
}

// Performance monitoring and optimization analysis
void analyzeSystemPerformance() {
  Logger::info("=== SYSTEM PERFORMANCE ANALYSIS ===");
  
  // Memory analysis
  size_t totalHeap = ESP.getHeapSize();
  size_t freeHeap = ESP.getFreeHeap();
  size_t totalPsram = psramFound() ? ESP.getPsramSize() : 0;
  size_t freePsram = psramFound() ? ESP.getFreePsram() : 0;
  
  Logger::info("ðŸ’¾ Memory Status:");
  Logger::info("  Heap: " + String(freeHeap / 1024) + " KB free / " + String(totalHeap / 1024) + " KB total (" + String((freeHeap * 100) / totalHeap) + "% free)");
  if (psramFound()) {
    Logger::info("  PSRAM: " + String(freePsram / 1024) + " KB free / " + String(totalPsram / 1024) + " KB total (" + String((freePsram * 100) / totalPsram) + "% free)");
  } else {
    Logger::warn("  PSRAM: Not available - performance will be limited");
  }
  
  // Color database analysis
  Logger::info("ðŸŽ¨ Color Database Performance:");
  size_t colorCount = simpleColorDB.isOpen() ? simpleColorDB.getColorCount() : fallbackColorCount;
  Logger::info("  Colors loaded: " + String(colorCount));
  
  // Search method analysis
  String activeMethod = "Basic Classification";
  String performanceNote = "Minimal functionality";
  
#if ENABLE_KDTREE
  if (settings.enableKdtree && kdTreeColorDB.isBuilt()) {
    activeMethod = "KD-Tree Search";
    float logN = log2(colorCount);
    performanceNote = "O(log " + String(colorCount) + ") â‰ˆ " + String(logN, 1) + " operations";
  } else
#endif
  if (simpleColorDB.isOpen()) {
    activeMethod = "Binary Database Search";
    performanceNote = "O(" + String(colorCount) + ") optimized operations";
  } else if (fallbackColorDatabase != nullptr) {
    activeMethod = "Fallback Database Search";
    performanceNote = "O(" + String(colorCount) + ") unoptimized operations";
  }
  
  Logger::info("  Active search method: " + activeMethod);
  Logger::info("  Performance complexity: " + performanceNote);
  
  // Performance optimization recommendations
  Logger::info("ðŸš€ Performance Recommendations:");
  
  if (colorCount <= 1000 && settings.enableKdtree) {
    Logger::info("  âœ… Small database - KD-tree overhead avoided (optimal)");
  } else if (colorCount > 1000 && !settings.enableKdtree) {
    Logger::warn("  âš ï¸ Large database without KD-tree - consider enabling for " + String((float)colorCount / log2(colorCount), 1) + "x speedup");
  } else if (colorCount > 1000 && settings.enableKdtree) {
    Logger::info("  âœ… Large database with KD-tree - optimal performance achieved");
  }
  
  if (freePsram < (PSRAM_SAFETY_MARGIN_KB * 1024)) {
    Logger::warn("  âš ï¸ Low PSRAM - increase safety margin or reduce database size");
  } else {
    Logger::info("  âœ… PSRAM adequate for current configuration");
  }
  
  if (freeHeap < 100000) {
    Logger::warn("  âš ï¸ Low heap memory - monitor for stability issues");
  } else {
    Logger::info("  âœ… Heap memory sufficient");
  }
  
  Logger::info("=====================================");
}

// Clean up color database memory
void cleanupColorDatabase() {
  Logger::debug("Cleaning up color database memory...");

  // Close simple binary database
  simpleColorDB.close();
  Logger::debug("Simple binary color database closed");

  // Clean up fallback database if allocated
  if (fallbackColorDatabase != nullptr) {
    Logger::debug("Cleaning up fallback color database memory...");
    heap_caps_free(fallbackColorDatabase);
    fallbackColorDatabase = nullptr;
    fallbackColorCount = 0;
    Logger::debug("Fallback color database memory cleaned up");
  }
}

// Global variables to store current sensor data
struct FastColorData {
  uint16_t x, y, z, ir1, ir2;
  uint8_t r, g, b;  // Integer values for web interface
  float r_precise, g_precise, b_precise;  // Float values for precision logging
  float batteryVoltage;  // Battery voltage in volts
  unsigned long timestamp;
};

struct FullColorData {
  FastColorData fast;
  String colorName;
  unsigned long colorNameTimestamp;
  unsigned long colorSearchDuration; // Time taken for color search in microseconds
} currentColorData;

// Color name lookup state
struct ColorNameLookup {
  bool inProgress = false;
  unsigned long lastLookupTime = 0;
  unsigned long lookupInterval = 2000; // Look up color name every 2 seconds
  uint8_t lastR = 0, lastG = 0, lastB = 0;
  bool needsUpdate = true;
  String currentColorName = "Initializing...";
} colorLookup;

// Handle root path - serve index.html
void handleRoot(AsyncWebServerRequest *request) {
  Logger::debug("Handling root path request");
  File file = LittleFS.open("/index.html", "r");
  if (!file) {
    Logger::error("index.html file not found");
    request->send(404, "text/plain", "File not found");
    return;
  }
  Logger::debug("Serving index.html");
  request->send(LittleFS, "/index.html", "text/html");
  file.close();
  Logger::debug("index.html served successfully");
}

// Handle CSS file
void handleCSS(AsyncWebServerRequest *request) {
  Logger::debug("Handling CSS file request");
  File file = LittleFS.open("/index.css", "r");
  if (!file) {
    Logger::error("index.css file not found");
    request->send(404, "text/plain", "File not found");
    return;
  }
  Logger::debug("Serving index.css");
  request->send(LittleFS, "/index.css", "text/css");
  file.close();
  Logger::debug("index.css served successfully");
}

// Handle JavaScript/TypeScript file
void handleJS(AsyncWebServerRequest *request) {
  Logger::debug("Handling JavaScript file request");
  File file = LittleFS.open("/index.js", "r");
  if (!file) {
    Logger::error("index.js file not found");
    request->send(404, "text/plain", "File not found");
    return;
  }
  Logger::debug("Serving index.js");
  request->send(LittleFS, "/index.js", "application/javascript");
  file.close();
  Logger::debug("index.js served successfully");
}

// Handle color API endpoint
void handleColorAPI(AsyncWebServerRequest *request) {
  Logger::debug("Handling color API request");
  // Create JSON response
  JsonDocument doc;  // ArduinoJson v7 syntax
  doc["r"] = currentColorData.fast.r;
  doc["g"] = currentColorData.fast.g;
  doc["b"] = currentColorData.fast.b;
  doc["x"] = currentColorData.fast.x;
  doc["y"] = currentColorData.fast.y;
  doc["z"] = currentColorData.fast.z;
  doc["ir1"] = currentColorData.fast.ir1;
  doc["ir2"] = currentColorData.fast.ir2;
  doc["colorName"] = currentColorData.colorName;
  doc["batteryVoltage"] = currentColorData.fast.batteryVoltage;
  doc["timestamp"] = currentColorData.fast.timestamp;

  String response;
  serializeJson(doc, response);
  Logger::debug("JSON response size: " + String(response.length()));
  
  // Add CORS headers for local development
  AsyncWebServerResponse *apiResponse = request->beginResponse(200, "application/json", response);
  apiResponse->addHeader("Access-Control-Allow-Origin", "*");
  request->send(apiResponse);
  Logger::debug("Color API response sent successfully");
}

// Handle fast color API endpoint (no color name lookup - optimized for speed)
void handleFastColorAPI(AsyncWebServerRequest *request) {
  Logger::debug("Handling fast color API request");
  // Create JSON response with only fast sensor data
  JsonDocument doc;
  doc["r"] = currentColorData.fast.r;
  doc["g"] = currentColorData.fast.g;
  doc["b"] = currentColorData.fast.b;
  doc["x"] = currentColorData.fast.x;
  doc["y"] = currentColorData.fast.y;
  doc["z"] = currentColorData.fast.z;
  doc["ir1"] = currentColorData.fast.ir1;
  doc["ir2"] = currentColorData.fast.ir2;
  doc["batteryVoltage"] = currentColorData.fast.batteryVoltage;
  doc["timestamp"] = currentColorData.fast.timestamp;

  String response;
  serializeJson(doc, response);
  
  // Add CORS headers
  AsyncWebServerResponse *apiResponse = request->beginResponse(200, "application/json", response);
  apiResponse->addHeader("Access-Control-Allow-Origin", "*");
  request->send(apiResponse);
  Logger::debug("Fast color API response sent successfully");
}

// Handle color name API endpoint (color name lookup only)
void handleColorNameAPI(AsyncWebServerRequest *request) {
  Logger::debug("Handling color name API request");
  // Create JSON response with color name information
  JsonDocument doc;
  doc["colorName"] = currentColorData.colorName;
  doc["colorNameTimestamp"] = currentColorData.colorNameTimestamp;
  doc["searchDuration"] = currentColorData.colorSearchDuration;
  doc["lookupInProgress"] = colorLookup.inProgress;
  doc["lastLookupTime"] = colorLookup.lastLookupTime;
  doc["lookupInterval"] = colorLookup.lookupInterval;
  
  // Include the RGB values that were used for the current color name
  doc["colorNameBasedOnR"] = colorLookup.lastR;
  doc["colorNameBasedOnG"] = colorLookup.lastG;
  doc["colorNameBasedOnB"] = colorLookup.lastB;

  String response;
  serializeJson(doc, response);
  
  // Add CORS headers
  AsyncWebServerResponse *apiResponse = request->beginResponse(200, "application/json", response);
  apiResponse->addHeader("Access-Control-Allow-Origin", "*");
  request->send(apiResponse);
  Logger::debug("Color name API response sent successfully");
}

// Handle force color lookup API endpoint (triggers immediate color name lookup)
void handleForceColorLookup(AsyncWebServerRequest *request) {
  Logger::debug("Handling force color lookup request");
  
  // Check if a lookup is already in progress
  if (colorLookup.inProgress) {
    request->send(429, "application/json", "{\"error\":\"Color lookup already in progress\",\"message\":\"Please wait for current lookup to complete\"}");
    return;
  }
  
  // Force immediate color lookup
  uint8_t currentR = currentColorData.fast.r;
  uint8_t currentG = currentColorData.fast.g;
  uint8_t currentB = currentColorData.fast.b;
  
  colorLookup.inProgress = true;
  unsigned long lookupStart = micros();
  
  String colorName = findClosestDuluxColor(currentR, currentG, currentB);
  unsigned long lookupDuration = micros() - lookupStart;
  
  // Update color name data
  unsigned long currentTime = millis();
  currentColorData.colorName = colorName;
  currentColorData.colorNameTimestamp = currentTime;
  currentColorData.colorSearchDuration = lookupDuration;
  colorLookup.currentColorName = colorName;
  colorLookup.lastLookupTime = currentTime;
  colorLookup.lastR = currentR;
  colorLookup.lastG = currentG;
  colorLookup.lastB = currentB;
  colorLookup.inProgress = false;
  
  // Create response
  JsonDocument doc;
  doc["colorName"] = colorName;
  doc["searchDuration"] = lookupDuration;
  doc["rgb"] = JsonDocument();
  doc["rgb"]["r"] = currentR;
  doc["rgb"]["g"] = currentG;
  doc["rgb"]["b"] = currentB;
  doc["timestamp"] = currentTime;
  doc["forced"] = true;

  String response;
  serializeJson(doc, response);
  
  // Add CORS headers
  AsyncWebServerResponse *apiResponse = request->beginResponse(200, "application/json", response);
  apiResponse->addHeader("Access-Control-Allow-Origin", "*");
  request->send(apiResponse);
  
  Logger::info("Forced color lookup: RGB(" + String(currentR) + "," + String(currentG) + "," + String(currentB) + 
               ") -> " + colorName + " | Duration: " + String(lookupDuration) + "Î¼s");
}

// Settings API handlers
void handleGetSettings(AsyncWebServerRequest *request) {
  Logger::debug("Handling get settings API request");
  JsonDocument doc;
  
  // Color Detection Settings
  doc["colorReadingSamples"] = settings.colorReadingSamples;
  doc["colorStabilityThreshold"] = settings.colorStabilityThreshold;
  doc["sensorSampleDelay"] = settings.sensorSampleDelay;
  doc["optimalSensorDistance"] = settings.optimalSensorDistance;
  
  // Sensor Hardware Settings
  doc["sensorIntegrationTime"] = settings.sensorIntegrationTime;
  doc["sensorSaturationThreshold"] = settings.sensorSaturationThreshold;
  doc["ledBrightness"] = settings.ledBrightness;
  
  // Color Calibration Settings
  doc["irCompensationFactor1"] = settings.irCompensationFactor1;
  doc["irCompensationFactor2"] = settings.irCompensationFactor2;
  doc["rgbSaturationLimit"] = settings.rgbSaturationLimit;
  
  // Calibration Mode
  doc["useDFRobotLibraryCalibration"] = settings.useDFRobotLibraryCalibration;
  doc["calibrationMode"] = settings.useDFRobotLibraryCalibration ? "dfrobot" : "custom";
  
  // Calibration Parameters
  doc["irCompensation"] = settings.irCompensation;
  doc["rSlope"] = settings.rSlope;
  doc["rOffset"] = settings.rOffset;
  doc["gSlope"] = settings.gSlope;
  doc["gOffset"] = settings.gOffset;
  doc["bSlope"] = settings.bSlope;
  doc["bOffset"] = settings.bOffset;
  
  // Yellow Detection Settings
  doc["yellowDistanceCompensation"] = settings.yellowDistanceCompensation;
  doc["yellowMinRatio"] = settings.yellowMinRatio;
  doc["yellowBrightnessThreshold"] = settings.yellowBrightnessThreshold;
  
  // Performance Settings
  doc["kdtreeMaxColors"] = settings.kdtreeMaxColors;
  doc["kdtreeSearchTimeout"] = settings.kdtreeSearchTimeout;
  doc["binarySearchTimeout"] = settings.binarySearchTimeout;
  doc["enableKdtree"] = settings.enableKdtree;
  
  // Debug Settings
  doc["debugSensorReadings"] = settings.debugSensorReadings;
  doc["debugColorMatching"] = settings.debugColorMatching;
  doc["debugMemoryUsage"] = settings.debugMemoryUsage;
  doc["debugPerformanceTiming"] = settings.debugPerformanceTiming;
  doc["sensorReadingInterval"] = settings.sensorReadingInterval;
  
  // Auto-adjust settings
  doc["enableAutoAdjust"] = settings.enableAutoAdjust;
  doc["autoSatHigh"] = settings.autoSatHigh;
  doc["autoSatLow"] = settings.autoSatLow;
  doc["minIntegrationTime"] = settings.minIntegrationTime;
  doc["maxIntegrationTime"] = settings.maxIntegrationTime;
  doc["integrationStep"] = settings.integrationStep;
  
  String response;
  serializeJson(doc, response);
  
  AsyncWebServerResponse *apiResponse = request->beginResponse(200, "application/json", response);
  apiResponse->addHeader("Access-Control-Allow-Origin", "*");
  request->send(apiResponse);
}

// Quick individual setting update endpoints for real-time control
void handleSetLedBrightness(AsyncWebServerRequest *request) {
  if (request->hasParam("value")) {
    int brightness = request->getParam("value")->value().toInt();
    if (brightness >= 0 && brightness <= 255) {
      settings.ledBrightness = brightness;
      analogWrite(LEDpin, settings.ledBrightness);
      Logger::info("LED brightness updated to: " + String(brightness));
      request->send(200, "application/json", "{\"status\":\"success\",\"brightness\":" + String(brightness) + "}");
    } else {
      request->send(400, "application/json", "{\"error\":\"Brightness must be 0-255\"}");
    }
  } else {
    request->send(400, "application/json", "{\"error\":\"Missing value parameter\"}");
  }
}

void handleSetIntegrationTime(AsyncWebServerRequest *request) {
  if (request->hasParam("value")) {
    int integrationTime = request->getParam("value")->value().toInt();
    if (integrationTime >= 0 && integrationTime <= 255) {
      settings.sensorIntegrationTime = integrationTime;
      TCS3430.setIntegrationTime(settings.sensorIntegrationTime);
      Logger::info("Integration time updated to: 0x" + String(integrationTime, HEX));
      request->send(200, "application/json", "{\"status\":\"success\",\"integrationTime\":" + String(integrationTime) + "}");
    } else {
      request->send(400, "application/json", "{\"error\":\"Integration time must be 0-255\"}");
    }
  } else {
    request->send(400, "application/json", "{\"error\":\"Missing value parameter\"}");
  }
}

void handleSetIRCompensation(AsyncWebServerRequest *request) {
  if (request->hasParam("ir1") && request->hasParam("ir2")) {
    float ir1 = request->getParam("ir1")->value().toFloat();
    float ir2 = request->getParam("ir2")->value().toFloat();
    if (ir1 >= 0 && ir1 <= 2.0 && ir2 >= 0 && ir2 <= 2.0) {
      settings.irCompensationFactor1 = ir1;
      settings.irCompensationFactor2 = ir2;
      Logger::info("IR compensation updated - IR1: " + String(ir1) + " IR2: " + String(ir2));
      request->send(200, "application/json", "{\"status\":\"success\",\"ir1\":" + String(ir1) + ",\"ir2\":" + String(ir2) + "}");
    } else {
      request->send(400, "application/json", "{\"error\":\"IR compensation factors must be 0-2.0\"}");
    }
  } else {
    request->send(400, "application/json", "{\"error\":\"Missing ir1 or ir2 parameters\"}");
  }
}

// This function applies the final calibration with optimized matrix precision
void convertXYZtoRGB_Calibrated(uint16_t X, uint16_t Y, uint16_t Z, uint16_t IR1, uint16_t IR2, uint8_t &R, uint8_t &G, uint8_t &B) {
  if (settings.debugSensorReadings) {
    Serial.print("[DEBUG] Converting XYZ to RGB - X:");
    Serial.print(X); Serial.print(" Y:"); Serial.print(Y); Serial.print(" Z:"); Serial.print(Z); 
    Serial.print(" IR1:"); Serial.print(IR1); Serial.print(" IR2:"); Serial.println(IR2);
  }

  // Check if we should use DFRobot library's built-in calibration
  if (settings.useDFRobotLibraryCalibration) {
    // Use DFRobot library's standard XYZ to RGB conversion matrix
    convertXYZtoRGB_Uncalibrated(X, Y, Z, R, G, B);
    
    if (settings.debugSensorReadings) {
      Serial.print("[DEBUG] DFRobot Library Result - R:");
      Serial.print(R); Serial.print(" G:"); Serial.print(G); Serial.print(" B:"); Serial.println(B);
    }
    return;
  }
  
  // Apply configurable IR compensation using runtime settings
  float localIR1Factor = settings.irCompensationFactor1;
  float localIR2Factor = settings.irCompensationFactor2;
  if (settings.enableDynamicIR) {
    if (IR1 > settings.irHighThreshold) {
      localIR1Factor = settings.irHighFactor;
      localIR2Factor = settings.irHighFactor;
    } else {
      localIR1Factor = settings.irLowFactor;
      localIR2Factor = settings.irLowFactor;
    }
  }
  float X_adj = X - (localIR1Factor * IR1) - (localIR2Factor * IR2);
  float Y_adj = Y - (localIR1Factor * IR1) - (localIR2Factor * IR2);
  float Z_adj = Z - (localIR1Factor * IR1) - (localIR2Factor * IR2);

  if (settings.debugSensorReadings) {
    Serial.print("[DEBUG] After IR compensation - X_adj:");
    Serial.print(X_adj); Serial.print(" Y_adj:"); Serial.print(Y_adj); Serial.print(" Z_adj:"); Serial.println(Z_adj);
    Serial.print("[DEBUG] IR compensation factors - IR1_factor:");
    Serial.print(localIR1Factor); Serial.print(" IR2_factor:"); Serial.println(localIR2Factor);
  }

  // === NEW MATRIX-BASED CALIBRATION (Primary Path) ===
  if (settings.useMatrixCalibration) {
    const float *M;
    const float *O;
    
    // Choose matrix based on brightness threshold
    if (settings.enableDynamicCalibration) {
      if (Y_adj > settings.dynamicThreshold) {
        M = settings.brightMatrix;
        O = settings.brightOffset;
        if (settings.debugSensorReadings) {
          Serial.println("[DEBUG] Using bright matrix calibration");
        }
      } else {
        M = settings.darkMatrix;
        O = settings.darkOffset;
        if (settings.debugSensorReadings) {
          Serial.println("[DEBUG] Using dark matrix calibration");
        }
      }
    } else {
      // Default to bright matrix
      M = settings.brightMatrix;
      O = settings.brightOffset;
      if (settings.debugSensorReadings) {
        Serial.println("[DEBUG] Using default bright matrix calibration");
      }
    }

    // Apply matrix transformation: RGB = M * XYZ + O
    float r_final = M[0]*X_adj + M[1]*Y_adj + M[2]*Z_adj + O[0];
    float g_final = M[3]*X_adj + M[4]*Y_adj + M[5]*Z_adj + O[1];
    float b_final = M[6]*X_adj + M[7]*Y_adj + M[8]*Z_adj + O[2];

    if (settings.debugSensorReadings) {
      Serial.print("[DEBUG] Matrix calculation - r:");
      Serial.print(r_final); Serial.print(" g:"); Serial.print(g_final); Serial.print(" b:"); Serial.println(b_final);
      Serial.print("[DEBUG] Matrix coefficients - M[0-2]:");
      Serial.print(M[0], 6); Serial.print(" "); Serial.print(M[1], 6); Serial.print(" "); Serial.println(M[2], 6);
      Serial.print("[DEBUG] Matrix coefficients - M[3-5]:");
      Serial.print(M[3], 6); Serial.print(" "); Serial.print(M[4], 6); Serial.print(" "); Serial.println(M[5], 6);
      Serial.print("[DEBUG] Matrix coefficients - M[6-8]:");
      Serial.print(M[6], 6); Serial.print(" "); Serial.print(M[7], 6); Serial.print(" "); Serial.println(M[8], 6);
      Serial.print("[DEBUG] Offset values - O[0-2]:");
      Serial.print(O[0], 2); Serial.print(" "); Serial.print(O[1], 2); Serial.print(" "); Serial.println(O[2], 2);
    }

    // Clamp to 0-RGB_SATURATION_LIMIT using configurable limit
    R = (uint8_t)max(0.0f, min((float)settings.rgbSaturationLimit, r_final));
    G = (uint8_t)max(0.0f, min((float)settings.rgbSaturationLimit, g_final));
    B = (uint8_t)max(0.0f, min((float)settings.rgbSaturationLimit, b_final));
    
    if (settings.debugSensorReadings) {
      Serial.print("[DEBUG] Final matrix RGB - R:");
      Serial.print(R); Serial.print(" G:"); Serial.print(G); Serial.print(" B:"); Serial.println(B);
    }
    return; // Matrix path complete
  }
  // === END MATRIX PATH ===

  // === LEGACY QUADRATIC CALIBRATION (Fallback) ===
  // Dynamic coefficient selection for quadratic calibration
  float redA_local = settings.redA;
  float redB_local = settings.redB;
  float redC_local = settings.redC;
  float greenA_local = settings.greenA;
  float greenB_local = settings.greenB;
  float greenC_local = settings.greenC;
  float blueA_local = settings.blueA;
  float blueB_local = settings.blueB;
  float blueC_local = settings.blueC;

  if (settings.enableDynamicCalibration) {
    if (Y_adj > settings.dynamicThreshold) {
      // Use white optimized coefficients
      redA_local = settings.whiteRedA;
      redB_local = settings.whiteRedB;
      redC_local = settings.whiteRedC;
      greenA_local = settings.whiteGreenA;
      greenB_local = settings.whiteGreenB;
      greenC_local = settings.whiteGreenC;
      blueA_local = settings.whiteBlueA;
      blueB_local = settings.whiteBlueB;
      blueC_local = settings.whiteBlueC;
    } else {
      // Use grey optimized coefficients
      redA_local = settings.greyRedA;
      redB_local = settings.greyRedB;
      redC_local = settings.greyRedC;
      greenA_local = settings.greyGreenA;
      greenB_local = settings.greyGreenB;
      greenC_local = settings.greyGreenC;
      blueA_local = settings.greyBlueA;
      blueB_local = settings.greyBlueB;
      blueC_local = settings.greyBlueC;
    }
  }

  // Use runtime adjustable quadratic parameters
  float r_final = redA_local * X_adj * X_adj + redB_local * X_adj + redC_local;
  float g_final = greenA_local * Y_adj * Y_adj + greenB_local * Y_adj + greenC_local;
  float b_final = blueA_local * Z_adj * Z_adj + blueB_local * Z_adj + blueC_local;

  if (settings.debugSensorReadings) {
    Serial.print("[DEBUG] Raw quadratic RGB calculations - r:");
    Serial.print(r_final); Serial.print(" g:"); Serial.print(g_final); Serial.print(" b:"); Serial.println(b_final);
    Serial.print("[DEBUG] Quadratic coefficients - Red(A:");
    Serial.print(redA_local, 10); Serial.print(" B:"); Serial.print(redB_local, 6); Serial.print(" C:"); Serial.print(redC_local, 2); Serial.println(")");
    Serial.print("[DEBUG] Green(A:");
    Serial.print(greenA_local, 10); Serial.print(" B:"); Serial.print(greenB_local, 6); Serial.print(" C:"); Serial.print(greenC_local, 2); Serial.println(")");
    Serial.print("[DEBUG] Blue(A:");
    Serial.print(blueA_local, 10); Serial.print(" B:"); Serial.print(blueB_local, 6); Serial.print(" C:"); Serial.print(blueC_local, 2); Serial.println(")");
  }

  // Clamp to 0-RGB_SATURATION_LIMIT using configurable limit
  R = (uint8_t)max(0.0f, min((float)settings.rgbSaturationLimit, r_final));
  G = (uint8_t)max(0.0f, min((float)settings.rgbSaturationLimit, g_final));
  B = (uint8_t)max(0.0f, min((float)settings.rgbSaturationLimit, b_final));
  
  if (settings.debugSensorReadings) {
    Serial.print("[DEBUG] Final clamped quadratic RGB - R:");
    Serial.print(R); Serial.print(" G:"); Serial.print(G); Serial.print(" B:"); Serial.println(B);
  }
}

// Uncalibrated function for comparison
void convertXYZtoRGB_Uncalibrated(uint16_t X, uint16_t Y, uint16_t Z, uint8_t &R, uint8_t &G, uint8_t &B) {
    float x = X / 65535.0f; float y = Y / 65535.0f; float z = Z / 65535.0f;
    float r_linear = 3.2406f*x-1.5372f*y-0.4986f*z; float g_linear=-0.9689f*x+1.8758f*y+0.0415f*z; float b_linear=0.0557f*x-0.2040f*y+1.0570f*z;
    r_linear=max(0.0f,min(1.0f,r_linear)); g_linear=max(0.0f,min(1.0f,g_linear)); b_linear=max(0.0f,min(1.0f,b_linear));
    float gamma=1.0f/2.2f; R=(uint8_t)(pow(r_linear,gamma)*255.0f); G=(uint8_t)(pow(g_linear,gamma)*255.0f); B=(uint8_t)(pow(b_linear,gamma)*255.0f);
}



// Battery voltage monitoring function for ProS3
// Get the battery voltage in volts
float getBatteryVoltage() {
  // Use official UMS3 library for accurate battery voltage reading
  // The library handles voltage dividers and ADC configuration automatically
  float batteryVoltage = ums3.getBatteryVoltage();
  
  // Log battery readings for debugging
  Serial.print("[BATTERY] ProS3 Official Library: ");
  Serial.print(batteryVoltage, 3);
  Serial.print("V");
  
  if (batteryVoltage < 0.5f) {
    Serial.println(" [WARNING: Very low reading - check battery connection/charge]");
  } else {
    Serial.println(" [Good reading]");
  }
  
  return batteryVoltage;
}

// Detect if VBUS (USB power) is present
bool getVbusPresent() {
  // Use official UMS3 library for VBUS detection
  // The library handles the hardware-specific implementation
  bool vbusPresent = ums3.getVbusPresent();
  
  Serial.print("[VBUS] ProS3 Official Library: USB ");
  Serial.println(vbusPresent ? "Present" : "Not Present");
  
  return vbusPresent;
}

// Battery API handler
void handleBatteryAPI(AsyncWebServerRequest *request) {
  Logger::debug("Handling battery API request");
  
  float batteryVoltage = getBatteryVoltage();
  bool usbPresent = getVbusPresent();
  
  // Create JSON response
  JsonDocument doc;
  doc["batteryVoltage"] = batteryVoltage;
  doc["timestamp"] = millis();
  doc["source"] = "adc_gpio1";  // Indicate this is from GPIO1 ADC reading
  doc["usbPowerPresent"] = usbPresent;
  
  // Battery status interpretation for LiPo batteries
  if (batteryVoltage > 4.0f) {
    doc["status"] = "excellent";
    doc["percentage"] = 100;
  } else if (batteryVoltage > 3.7f) {
    doc["status"] = "good";
    doc["percentage"] = (int)((batteryVoltage - 3.0f) / 1.2f * 100);
  } else if (batteryVoltage > 3.4f) {
    doc["status"] = "low";
    doc["percentage"] = (int)((batteryVoltage - 3.0f) / 1.2f * 100);
  } else {
    doc["status"] = "critical";
    doc["percentage"] = 0;
  }
  
  // Determine power source
  if (usbPresent && batteryVoltage > 2.5f) {
    doc["powerSource"] = "usb_and_battery";
  } else if (usbPresent) {
    doc["powerSource"] = "usb_only";
  } else if (batteryVoltage > 2.5f) {
    doc["powerSource"] = "battery_only";
  } else {
    doc["powerSource"] = "unknown";
  }

  String response;
  serializeJson(doc, response);
  
  AsyncWebServerResponse *apiResponse = request->beginResponse(200, "application/json", response);
  apiResponse->addHeader("Access-Control-Allow-Origin", "*");
  request->send(apiResponse);
  
  Logger::debug("Battery API response sent: " + String(batteryVoltage, 3) + "V (GPIO1 ADC)");
}

// Function to scan for WiFi networks and start AP mode if target SSID not found
bool connectToWiFiOrStartAP() {
  Logger::info("Scanning for available WiFi networks...");
  
  // Scan for networks
  int numNetworks = WiFi.scanNetworks();
  bool targetFound = false;
  
  if (numNetworks == 0) {
    Logger::warn("No WiFi networks found");
  } else {
    Logger::info("Found " + String(numNetworks) + " WiFi networks:");
    for (int i = 0; i < numNetworks; i++) {
      String foundSSID = WiFi.SSID(i);
      Logger::info("  " + String(i + 1) + ": " + foundSSID + " (Signal: " + String(WiFi.RSSI(i)) + " dBm)");
      
      if (foundSSID == String(ssid)) {
        targetFound = true;
        Logger::info("Target SSID '" + String(ssid) + "' found!");
      }
    }
  }
  
  if (!targetFound) {
    Logger::warn("Target SSID '" + String(ssid) + "' not found in scan results");
    Logger::info("Starting Access Point mode...");
    
    // Start AP mode
    WiFi.mode(WIFI_AP);
    bool apStarted = WiFi.softAP(ap_ssid, ap_password);
    
    if (apStarted) {
      IPAddress apIP = WiFi.softAPIP();
      Logger::info("Access Point started successfully!");
      Logger::info("AP SSID: " + String(ap_ssid));
      Logger::info("AP Password: " + String(ap_password));
      Logger::info("AP IP address: " + apIP.toString());
      Logger::info("Connect to the AP and visit http://" + apIP.toString() + " to access the color matcher");
      return true; // AP mode started successfully
    } else {
      Logger::error("Failed to start Access Point mode!");
      return false;
    }
  }
  
  // Target SSID found, attempt to connect
  Logger::info("Attempting to connect to WiFi network: " + String(ssid));
  
  // Configure static IP for station mode
  WiFi.mode(WIFI_STA);
  if (!WiFi.config(local_IP, gateway, subnet)) {
    Logger::error("Static IP configuration failed, using DHCP");
  } else {
    Logger::debug("Static IP configuration successful");
  }
  
  WiFi.begin(ssid, password);
  
  unsigned long wifiStartTime = millis();
  const unsigned long wifiTimeout = WIFI_TIMEOUT_MS; // Configurable WiFi timeout
  
  Logger::info("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED && (millis() - wifiStartTime) < wifiTimeout) {
    delay(500);
    Serial.print(".");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Logger::info("WiFi connected successfully!");
    Logger::info("IP address: " + WiFi.localIP().toString());
    Logger::info("Connection time: " + String((millis() - wifiStartTime) / 1000) + " seconds");
    return true;
  } else {
    Serial.println();
    Logger::warn("WiFi connection failed after timeout, starting Access Point mode...");
    
    // Start AP mode as fallback
    WiFi.mode(WIFI_AP);
    bool apStarted = WiFi.softAP(ap_ssid, ap_password);
    
    if (apStarted) {
      IPAddress apIP = WiFi.softAPIP();
      Logger::info("Fallback Access Point started successfully!");
      Logger::info("AP SSID: " + String(ap_ssid));
      Logger::info("AP Password: " + String(ap_password));
      Logger::info("AP IP address: " + apIP.toString());
      Logger::info("Connect to the AP and visit http://" + apIP.toString() + " to access the color matcher");
      return true;
    } else {
      Logger::error("Failed to start fallback Access Point mode!");
      return false;
    }
  }
}

void setup() {
  Serial.begin(115200);
  Logger::info("System startup initiated - Serial communication started at 115200 baud");
  
  // Initialize IP addresses from string settings
  initializeIPAddresses();

  // Check if PSRAM is available and properly configured
  if (psramFound()) {
    size_t psramSize = ESP.getPsramSize();
    size_t freePsram = ESP.getFreePsram();
    Logger::info("PSRAM detected and available");
    Logger::info("PSRAM total size: " + String(psramSize / 1024) + " KB");
    Logger::info("PSRAM free size: " + String(freePsram / 1024) + " KB");
    Logger::info("PSRAM usage: " + String(((psramSize - freePsram) * 100) / psramSize) + "%");
    
    // Verify we have enough PSRAM for color database
    if (freePsram < 2 * 1024 * 1024) { // Less than 2MB free
      Logger::warn("Low PSRAM available: " + String(freePsram / 1024) + " KB - color database may use fallback");
    }
  } else {
    Logger::error("PSRAM not found! This will severely limit color database functionality.");
    Logger::error("Please check hardware configuration and PSRAM initialization.");
  }
  
  // Log total heap information
  Logger::info("Total heap size: " + String(ESP.getHeapSize() / 1024) + " KB");
  Logger::info("Free heap size: " + String(ESP.getFreeHeap() / 1024) + " KB");

  // Display current settings configuration
  displayCurrentSettings();

  // Initialize I2C with custom pins for ESP32-S3 ProS3
  Wire.begin(SDA_PIN, SCL_PIN);
  Logger::debug("I2C initialized with SDA=3, SCL=4");

  pinMode(LEDpin, OUTPUT);
  // Set the LED brightness from runtime settings
  analogWrite(LEDpin, settings.ledBrightness);
  Logger::debug("LED pin configured, brightness set to: " + String(settings.ledBrightness));

  // Initialize UMS3 library for ProS3 board peripherals
  ums3.begin();
  Logger::debug("UMS3 library initialized - ProS3 board peripherals ready");
  
  // Set NeoPixel brightness to 1/3 for battery status indication
  ums3.setPixelBrightness(255 / 3);
  Logger::debug("NeoPixel brightness set for battery status indication");
  
  // Log initial battery voltage using official library
  float initialBatteryVoltage = getBatteryVoltage();
  Logger::info("Initial battery voltage: " + String(initialBatteryVoltage, 3) + "V");

  Logger::info("Initializing TCS3430 sensor...");

  while (!TCS3430.begin()) {
    Logger::error("TCS3430 sensor initialization failed, retrying in 1 second...");
    delay(1000);
  }
  Logger::info("TCS3430 sensor initialized successfully");

  // Standard sensor configuration
  Logger::debug("Configuring sensor parameters...");
  TCS3430.setAutoZeroMode(1);
  Logger::debug("Auto zero mode set to 1");
  TCS3430.setAutoZeroNTHIteration(0);
  Logger::debug("Auto zero NTH iteration set to 0");
  TCS3430.setHighGAIN(false);  // Disable high gain to prevent saturation
  Logger::debug("High gain disabled to prevent saturation on bright colors");
  TCS3430.setIntegrationTime(settings.sensorIntegrationTime);  // Use runtime setting
  Logger::debug("Integration time set to 0x" + String(settings.sensorIntegrationTime, HEX));
  TCS3430.setALSGain(2);  // Reduced to 16x gain for better linearity and less artifacts
  Logger::debug("ALS gain set to 2 (16x) for optimal accuracy");

  Logger::info("Sensor ready with final calibration parameters loaded");

  // Initialize LittleFS
  Logger::info("Initializing LittleFS filesystem...");

  // Use "spiffs" partition label (compatible with LittleFS library)
  if (!LittleFS.begin(false, "/littlefs", 10, "spiffs")) {
    Logger::warn("LittleFS mount failed, attempting to format...");
    if (LittleFS.format()) {  // Format the filesystem
      Logger::info("LittleFS formatted successfully");
      delay(100); // Small delay after format
      if (LittleFS.begin(false, "/littlefs", 10, "spiffs")) {
        Logger::info("LittleFS mounted after format");
      } else {
        Logger::error("LittleFS mount failed even after format! System may be unstable.");
        // Continue with no FS, or fallback mode
      }
    } else {
      Logger::error("LittleFS format failed! Check partition config.");
      // Continue anyway - system may still work without filesystem
    }
  } else {
    Logger::info("LittleFS mounted successfully");
  }

  // Log filesystem space information
  size_t total = LittleFS.totalBytes();
  size_t used = LittleFS.usedBytes();
  Logger::info("LittleFS Total: " + String(total / 1024) + " KB");
  Logger::info("LittleFS Used: " + String(used / 1024) + " KB");
  Logger::info("LittleFS Free: " + String((total - used) / 1024) + " KB");

  // Load color database
  Logger::info("Loading color database...");

  // File listing (commented out to speed boot - uncomment for debugging)
  /*
  Logger::info("Checking available files in LittleFS...");
  File root = LittleFS.open("/");
  if (root) {
    File file = root.openNextFile();
    int fileCount = 0;
    while (file && fileCount < 10) {
      Logger::info("Found file: " + String(file.name()) + " (" + String(file.size()) + " bytes)");
      file = root.openNextFile();
      fileCount++;
    }
    root.close();
    Logger::info("File enumeration complete, found " + String(fileCount) + " files");
  }
  */
  
  if (loadColorDatabase()) {
    Logger::info("Color database loaded successfully");
    
    // Monitor memory usage after loading
    Logger::debug("Free heap after loading: " + String(esp_get_free_heap_size()));
    Logger::debug("Free PSRAM after loading: " + String(ESP.getFreePsram()));
    
    // Perform performance analysis and optimization recommendations
    analyzeSystemPerformance();
  } else {
    Logger::error("Could not load color database - color matching may not work");
  }

  // Connect to WiFi or start AP mode
  if (!connectToWiFiOrStartAP()) {
    Logger::error("Failed to establish network connectivity - system may not function properly");
  }

  // Serve static files
  Logger::debug("Setting up web server routes...");
  server.on("/", HTTP_GET, handleRoot);
  Logger::debug("Route registered: / -> handleRoot");
  server.on("/index.html", HTTP_GET, handleRoot);
  Logger::debug("Route registered: /index.html -> handleRoot");
  server.on("/index.css", HTTP_GET, handleCSS);
  Logger::debug("Route registered: /index.css -> handleCSS");
  server.on("/index.js", HTTP_GET, handleJS);
  Logger::debug("Route registered: /index.js -> handleJS");
  server.on("/api/color", HTTP_GET, handleColorAPI);
  Logger::debug("Route registered: /api/color -> handleColorAPI");

  // Fast live feed and color name API endpoints
  server.on("/api/color-fast", HTTP_GET, handleFastColorAPI);
  Logger::debug("Route registered: /api/color-fast -> handleFastColorAPI (fast sensor data only)");
  server.on("/api/color-name", HTTP_GET, handleColorNameAPI);
  Logger::debug("Route registered: /api/color-name -> handleColorNameAPI (color name lookup only)");
  server.on("/api/force-color-lookup", HTTP_GET, handleForceColorLookup);
  Logger::debug("Route registered: /api/force-color-lookup -> handleForceColorLookup (immediate color name lookup)");
  
  // Battery monitoring API endpoint
  server.on("/api/battery", HTTP_GET, handleBatteryAPI);
  Logger::debug("Route registered: /api/battery -> handleBatteryAPI (battery voltage monitoring)");
  


  // Settings API endpoints - Simplified approach
  server.on("/api/settings", HTTP_GET, handleGetSettings);
  Logger::debug("Route registered: /api/settings (GET) -> handleGetSettings");
  
  // Individual setting update endpoints for real-time adjustment (GET method for simplicity)
  server.on("/api/set-led-brightness", HTTP_GET, handleSetLedBrightness);
  Logger::debug("Route registered: /api/set-led-brightness (GET) -> handleSetLedBrightness");
  
  server.on("/api/set-integration-time", HTTP_GET, handleSetIntegrationTime);
  Logger::debug("Route registered: /api/set-integration-time (GET) -> handleSetIntegrationTime");
  
  server.on("/api/set-ir-factors", HTTP_GET, handleSetIRCompensation);
  Logger::debug("Route registered: /api/set-ir-factors (GET) -> handleSetIRCompensation");
  
  server.on("/api/set-color-samples", HTTP_GET, handleSetColorSamples);
  Logger::debug("Route registered: /api/set-color-samples (GET) -> handleSetColorSamples");
  
  server.on("/api/set-sample-delay", HTTP_GET, handleSetSampleDelay);
  Logger::debug("Route registered: /api/set-sample-delay (GET) -> handleSetSampleDelay");
  
  server.on("/api/set-debug", HTTP_GET, handleSetDebugSettings);
  Logger::debug("Route registered: /api/set-debug (GET) -> handleSetDebugSettings");

  // Calibration API endpoints
  server.on("/api/calibration", HTTP_GET, handleGetCalibration);
  Logger::debug("Route registered: /api/calibration (GET) -> handleGetCalibration");
  
  server.on("/api/calibration", HTTP_POST, 
    [](AsyncWebServerRequest *request){
      // Simple GET-style parameter handling for calibration updates
      bool updated = false;
      String response = "{\"status\":\"success\"";
      
      // Handle individual coefficient updates via query parameters
      if (request->hasParam("redA")) {
        settings.redA = request->getParam("redA")->value().toFloat();
        response += ",\"redA\":" + String(settings.redA, 10);
        updated = true;
      }
      if (request->hasParam("redB")) {
        settings.redB = request->getParam("redB")->value().toFloat();
        response += ",\"redB\":" + String(settings.redB, 6);
        updated = true;
      }
      if (request->hasParam("redC")) {
        settings.redC = request->getParam("redC")->value().toFloat();
        response += ",\"redC\":" + String(settings.redC, 2);
        updated = true;
      }
      if (request->hasParam("greenA")) {
        settings.greenA = request->getParam("greenA")->value().toFloat();
        response += ",\"greenA\":" + String(settings.greenA, 10);
        updated = true;
      }
      if (request->hasParam("greenB")) {
        settings.greenB = request->getParam("greenB")->value().toFloat();
        response += ",\"greenB\":" + String(settings.greenB, 6);
        updated = true;
      }
      if (request->hasParam("greenC")) {
        settings.greenC = request->getParam("greenC")->value().toFloat();
        response += ",\"greenC\":" + String(settings.greenC, 2);
        updated = true;
      }
      if (request->hasParam("blueA")) {
        settings.blueA = request->getParam("blueA")->value().toFloat();
        response += ",\"blueA\":" + String(settings.blueA, 10);
        updated = true;
      }
      if (request->hasParam("blueB")) {
        settings.blueB = request->getParam("blueB")->value().toFloat();
        response += ",\"blueB\":" + String(settings.blueB, 6);
        updated = true;
      }
      if (request->hasParam("blueC")) {
        settings.blueC = request->getParam("blueC")->value().toFloat();
        response += ",\"blueC\":" + String(settings.blueC, 2);
        updated = true;
      }
      // White coefficients
      if (request->hasParam("whiteRedA")) {
        settings.whiteRedA = request->getParam("whiteRedA")->value().toFloat();
        response += ",\"whiteRedA\":" + String(settings.whiteRedA, 10);
        updated = true;
      }
      if (request->hasParam("whiteRedB")) {
        settings.whiteRedB = request->getParam("whiteRedB")->value().toFloat();
        response += ",\"whiteRedB\":" + String(settings.whiteRedB, 6);
        updated = true;
      }
      if (request->hasParam("whiteRedC")) {
        settings.whiteRedC = request->getParam("whiteRedC")->value().toFloat();
        response += ",\"whiteRedC\":" + String(settings.whiteRedC, 2);
        updated = true;
      }
      if (request->hasParam("whiteGreenA")) {
        settings.whiteGreenA = request->getParam("whiteGreenA")->value().toFloat();
        response += ",\"whiteGreenA\":" + String(settings.whiteGreenA, 10);
        updated = true;
      }
      if (request->hasParam("whiteGreenB")) {
        settings.whiteGreenB = request->getParam("whiteGreenB")->value().toFloat();
        response += ",\"whiteGreenB\":" + String(settings.whiteGreenB, 6);
        updated = true;
      }
      if (request->hasParam("whiteGreenC")) {
        settings.whiteGreenC = request->getParam("whiteGreenC")->value().toFloat();
        response += ",\"whiteGreenC\":" + String(settings.whiteGreenC, 2);
        updated = true;
      }
      if (request->hasParam("whiteBlueA")) {
        settings.whiteBlueA = request->getParam("whiteBlueA")->value().toFloat();
        response += ",\"whiteBlueA\":" + String(settings.whiteBlueA, 10);
        updated = true;
      }
      if (request->hasParam("whiteBlueB")) {
        settings.whiteBlueB = request->getParam("whiteBlueB")->value().toFloat();
        response += ",\"whiteBlueB\":" + String(settings.whiteBlueB, 6);
        updated = true;
      }
      if (request->hasParam("whiteBlueC")) {
        settings.whiteBlueC = request->getParam("whiteBlueC")->value().toFloat();
        response += ",\"whiteBlueC\":" + String(settings.whiteBlueC, 2);
        updated = true;
      }
      // Grey coefficients
      if (request->hasParam("greyRedA")) {
        settings.greyRedA = request->getParam("greyRedA")->value().toFloat();
        response += ",\"greyRedA\":" + String(settings.greyRedA, 10);
        updated = true;
      }
      if (request->hasParam("greyRedB")) {
        settings.greyRedB = request->getParam("greyRedB")->value().toFloat();
        response += ",\"greyRedB\":" + String(settings.greyRedB, 6);
        updated = true;
      }
      if (request->hasParam("greyRedC")) {
        settings.greyRedC = request->getParam("greyRedC")->value().toFloat();
        response += ",\"greyRedC\":" + String(settings.greyRedC, 2);
        updated = true;
      }
      if (request->hasParam("greyGreenA")) {
        settings.greyGreenA = request->getParam("greyGreenA")->value().toFloat();
        response += ",\"greyGreenA\":" + String(settings.greyGreenA, 10);
        updated = true;
      }
      if (request->hasParam("greyGreenB")) {
        settings.greyGreenB = request->getParam("greyGreenB")->value().toFloat();
        response += ",\"greyGreenB\":" + String(settings.greyGreenB, 6);
        updated = true;
      }
      if (request->hasParam("greyGreenC")) {
        settings.greyGreenC = request->getParam("greyGreenC")->value().toFloat();
        response += ",\"greyGreenC\":" + String(settings.greyGreenC, 2);
        updated = true;
      }
      if (request->hasParam("greyBlueA")) {
        settings.greyBlueA = request->getParam("greyBlueA")->value().toFloat();
        response += ",\"greyBlueA\":" + String(settings.greyBlueA, 10);
        updated = true;
      }
      if (request->hasParam("greyBlueB")) {
        settings.greyBlueB = request->getParam("greyBlueB")->value().toFloat();
        response += ",\"greyBlueB\":" + String(settings.greyBlueB, 6);
        updated = true;
      }
      if (request->hasParam("greyBlueC")) {
        settings.greyBlueC = request->getParam("greyBlueC")->value().toFloat();
        response += ",\"greyBlueC\":" + String(settings.greyBlueC, 2);
        updated = true;
      }
      // Dynamic settings
      if (request->hasParam("enableDynamicCalibration")) {
        settings.enableDynamicCalibration = request->getParam("enableDynamicCalibration")->value() == "true";
        response += ",\"enableDynamicCalibration\":" + String(settings.enableDynamicCalibration ? "true" : "false");
        updated = true;
      }
      if (request->hasParam("dynamicThreshold")) {
        settings.dynamicThreshold = request->getParam("dynamicThreshold")->value().toFloat();
        response += ",\"dynamicThreshold\":" + String(settings.dynamicThreshold, 2);
        updated = true;
      }
      
      // Matrix calibration parameters
      if (request->hasParam("useMatrixCalibration")) {
        settings.useMatrixCalibration = request->getParam("useMatrixCalibration")->value() == "true";
        response += ",\"useMatrixCalibration\":" + String(settings.useMatrixCalibration ? "true" : "false");
        updated = true;
      }
      // Helper lambda to update array elements
      auto updateMatrixArray = [&](const char *prefix, float *arr, size_t len){
        for (size_t idx = 0; idx < len; ++idx) {
          String key = String(prefix) + String(idx);
          if (request->hasParam(key)) {
            arr[idx] = request->getParam(key)->value().toFloat();
            response += ",\"" + key + "\":" + String(arr[idx], 6);
            updated = true;
          }
        }
      };
      updateMatrixArray("brightMatrix", settings.brightMatrix, 9);
      updateMatrixArray("brightOffset", settings.brightOffset, 3);
      updateMatrixArray("darkMatrix", settings.darkMatrix, 9);
      updateMatrixArray("darkOffset", settings.darkOffset, 3);
      
      response += "}";
      
      if (updated) {
        Logger::info("Calibration coefficients updated");
        Logger::info("Red: A=" + String(settings.redA, 10) + " B=" + String(settings.redB, 6) + " C=" + String(settings.redC, 2));
        Logger::info("Green: A=" + String(settings.greenA, 10) + " B=" + String(settings.greenB, 6) + " C=" + String(settings.greenC, 2));
        Logger::info("Blue: A=" + String(settings.blueA, 10) + " B=" + String(settings.blueB, 6) + " C=" + String(settings.blueC, 2));
        
        AsyncWebServerResponse *apiResponse = request->beginResponse(200, "application/json", response);
        apiResponse->addHeader("Access-Control-Allow-Origin", "*");
        request->send(apiResponse);
      } else {
        request->send(400, "application/json", "{\"error\":\"No valid calibration parameters provided\"}");
      }
    });
  Logger::debug("Route registered: /api/calibration (POST) -> handleSetCalibration");
  
  server.on("/api/tune-vivid-white", HTTP_POST, handleTuneVividWhite);
  Logger::debug("Route registered: /api/tune-vivid-white (POST) -> handleTuneVividWhite");

  // Reset and calibration mode endpoints
  server.on("/api/reset-to-dfrobot", HTTP_POST, [](AsyncWebServerRequest *request){
    // Reset to DFRobot library defaults
    settings.useDFRobotLibraryCalibration = true;
    settings.ledBrightness = LED_BRIGHTNESS;
    settings.sensorIntegrationTime = SENSOR_INTEGRATION_TIME;
    settings.colorReadingSamples = COLOR_READING_SAMPLES;
    settings.sensorSampleDelay = SENSOR_SAMPLE_DELAY;
    settings.irCompensationFactor1 = IR_COMPENSATION_FACTOR_1;
    settings.irCompensationFactor2 = IR_COMPENSATION_FACTOR_2;
    
    Logger::info("Settings reset to DFRobot library defaults");
    request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"Reset to DFRobot library defaults\",\"mode\":\"dfrobot\"}");
  });
  Logger::debug("Route registered: /api/reset-to-dfrobot (POST) -> reset to DFRobot defaults");

  server.on("/api/reset-to-custom", HTTP_POST, [](AsyncWebServerRequest *request){
    // Reset to custom quadratic calibration defaults
    settings.useDFRobotLibraryCalibration = false;
    settings.redA = 5.756615248518086e-06f;
    settings.redB = -0.10824971353127427f;
    settings.redC = 663.2283515839658f;
    settings.greenA = 7.700364703908128e-06f;
    settings.greenB = -0.14873455804115546f;
    settings.greenC = 855.288778468652f;
    settings.blueA = -2.7588632792769936e-06f;
    settings.blueB = 0.04959423885676833f;
    settings.blueC = 35.55576869603341f;
    
    Logger::info("Settings reset to custom quadratic calibration defaults");
    request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"Reset to custom calibration defaults\",\"mode\":\"custom\"}");
  });
  Logger::debug("Route registered: /api/reset-to-custom (POST) -> reset to custom defaults");

  server.on("/api/set-calibration-mode", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("mode")) {
      String mode = request->getParam("mode")->value();
      if (mode == "dfrobot") {
        settings.useDFRobotLibraryCalibration = true;
        Logger::info("Calibration mode set to DFRobot library");
        request->send(200, "application/json", "{\"status\":\"success\",\"mode\":\"dfrobot\"}");
      } else if (mode == "custom") {
        settings.useDFRobotLibraryCalibration = false;
        Logger::info("Calibration mode set to custom quadratic");
        request->send(200, "application/json", "{\"status\":\"success\",\"mode\":\"custom\"}");
      } else {
        request->send(400, "application/json", "{\"error\":\"Invalid mode. Use 'dfrobot' or 'custom'\"}");
      }
    } else {
      request->send(400, "application/json", "{\"error\":\"Missing mode parameter\"}");
    }
  });
  Logger::debug("Route registered: /api/set-calibration-mode (GET) -> set calibration mode");

  // Debug endpoint for testing
  server.on("/api/debug", HTTP_GET, [](AsyncWebServerRequest *request){
    String response = "{\"status\":\"ok\",\"message\":\"ESP32 API is working\",\"timestamp\":" + String(millis()) + "}";
    AsyncWebServerResponse *apiResponse = request->beginResponse(200, "application/json", response);
    apiResponse->addHeader("Access-Control-Allow-Origin", "*");
    request->send(apiResponse);
  });
  Logger::debug("Route registered: /api/debug -> debug endpoint");

  // Handle not found
  server.onNotFound([](AsyncWebServerRequest *request){
    Logger::debug("404 request received");
    request->send(404, "text/plain", "Not found");
  });
  Logger::debug("404 handler registered");

  server.begin();
  Logger::info("Web server started and listening on port 80");
  Logger::info("=== SYSTEM INITIALIZATION COMPLETE ===");
}

// Function to display current settings (defined here since Serial is available)
void displayCurrentSettings() {
    Serial.println("=== COLOR SENSOR SETTINGS ===");
    Serial.printf("WiFi: %s | Port: %d\n", WIFI_SSID, WEB_SERVER_PORT);
    Serial.printf("Static IP: %s | Gateway: %s\n", STATIC_IP, GATEWAY_IP);
    Serial.printf("KD-Tree: %s | Max Colors: %d\n", 
                  settings.enableKdtree ? "ENABLED" : "DISABLED", settings.kdtreeMaxColors);
    Serial.printf("Color Samples: %d | Stability: %d\n", 
                  settings.colorReadingSamples, settings.colorStabilityThreshold);
    Serial.printf("Sensor Distance: %dmm | LED Brightness: %d\n",
                  settings.optimalSensorDistance, settings.ledBrightness);
    Serial.printf("IR Compensation: IR1=%.3f | IR2=%.3f | RGB Limit=%d\n",
                  settings.irCompensationFactor1, settings.irCompensationFactor2, settings.rgbSaturationLimit);
    Serial.printf("Integration Time: 0x%02X | Debug Level: %s\n", 
                  settings.sensorIntegrationTime, settings.debugSensorReadings ? "DETAILED" : "BASIC");
    Serial.println("=============================");
}

void loop() {
  // AsyncWebServer handles requests automatically, no need for handleClient()
  
  // Read and average sensor data using runtime settings
  const int num_samples = settings.colorReadingSamples;
  uint32_t sumX = 0, sumY = 0, sumZ = 0, sumIR1 = 0, sumIR2 = 0;
  for (int i = 0; i < num_samples; i++) {
    sumX += TCS3430.getXData();
    sumY += TCS3430.getYData();
    sumZ += TCS3430.getZData();
    sumIR1 += TCS3430.getIR1Data();
    sumIR2 += TCS3430.getIR2Data();
    delay(settings.sensorSampleDelay);  // Use runtime setting
  }
  uint16_t XData = sumX / num_samples;
  uint16_t YData = sumY / num_samples;
  uint16_t ZData = sumZ / num_samples;
  uint16_t IR1Data = sumIR1 / num_samples;
  uint16_t IR2Data = sumIR2 / num_samples;

  // Auto-adjust integration time if enabled
  if (settings.enableAutoAdjust) {
    uint16_t maxChannel = max(max(XData, YData), ZData);
    float satLevel = (float)maxChannel / settings.sensorSaturationThreshold;
    if (satLevel > settings.autoSatHigh && settings.sensorIntegrationTime > settings.minIntegrationTime) {
      int newTime = (int)settings.sensorIntegrationTime - (int)settings.integrationStep;
      settings.sensorIntegrationTime = (uint8_t)max((int)settings.minIntegrationTime, newTime);
      TCS3430.setIntegrationTime(settings.sensorIntegrationTime);
      if (settings.debugSensorReadings) Serial.println("[AUTO] Decreased integration to " + String(settings.sensorIntegrationTime, HEX));
    } else if (satLevel < settings.autoSatLow && settings.sensorIntegrationTime < settings.maxIntegrationTime) {
      int newTime = (int)settings.sensorIntegrationTime + (int)settings.integrationStep;
      settings.sensorIntegrationTime = (uint8_t)min((int)settings.maxIntegrationTime, newTime);
      TCS3430.setIntegrationTime(settings.sensorIntegrationTime);
      if (settings.debugSensorReadings) Serial.println("[AUTO] Increased integration to " + String(settings.sensorIntegrationTime, HEX));
    }
  }

  uint8_t R, G, B;

  // Convert to RGB using quadratic calibration with IR1 and IR2 compensation
  convertXYZtoRGB_Calibrated(XData, YData, ZData, IR1Data, IR2Data, R, G, B);

  // Minimal smoothing filter for maximum color accuracy
  static float smoothed_R = R, smoothed_G = G, smoothed_B = B;
  const float smoothing_factor = 0.05f;  // Very light smoothing to reduce artifacts without lag
  smoothed_R = smoothed_R * smoothing_factor + R * (1.0f - smoothing_factor);
  smoothed_G = smoothed_G * smoothing_factor + G * (1.0f - smoothing_factor);
  smoothed_B = smoothed_B * smoothing_factor + B * (1.0f - smoothing_factor);

  // Read battery voltage
  float batteryVoltage = getBatteryVoltage();

  // Update current fast color data for API (use smoothed values) - NO COLOR NAME SEARCH
  currentColorData.fast = {XData, YData, ZData, IR1Data, IR2Data,
                      (uint8_t)round(smoothed_R), (uint8_t)round(smoothed_G), (uint8_t)round(smoothed_B),  // Integer values for web
                      smoothed_R, smoothed_G, smoothed_B,  // Float values for precision
                      batteryVoltage,  // Battery voltage
                      millis()};

  // Separate color name lookup - runs independently on a timer basis
  unsigned long currentTime = millis();
  if (!colorLookup.inProgress && (currentTime - colorLookup.lastLookupTime > colorLookup.lookupInterval)) {
    // Check if RGB values changed significantly (threshold of 5 to avoid constant lookup)
    uint8_t currentR = (uint8_t)round(smoothed_R);
    uint8_t currentG = (uint8_t)round(smoothed_G);
    uint8_t currentB = (uint8_t)round(smoothed_B);
    
    int rgbDiff = abs(currentR - colorLookup.lastR) + abs(currentG - colorLookup.lastG) + abs(currentB - colorLookup.lastB);
    
    if (colorLookup.needsUpdate || rgbDiff > 5) {
      colorLookup.inProgress = true;
      colorLookup.lastLookupTime = currentTime;
      colorLookup.lastR = currentR;
      colorLookup.lastG = currentG;
      colorLookup.lastB = currentB;
      
      // Perform color name lookup (this is the expensive operation)
      unsigned long colorSearchStart = micros();
      String colorName = findClosestDuluxColor(currentR, currentG, currentB);
      unsigned long colorSearchTime = micros() - colorSearchStart;
      
      // Update color name data
      currentColorData.colorName = colorName;
      currentColorData.colorNameTimestamp = currentTime;
      currentColorData.colorSearchDuration = colorSearchTime;
      colorLookup.currentColorName = colorName;
      colorLookup.needsUpdate = false;
      colorLookup.inProgress = false;
      
      if (settings.debugColorMatching) {
        String searchMethod = "Fallback";
#if ENABLE_KDTREE
        if (settings.enableKdtree && kdTreeColorDB.isBuilt()) {
          searchMethod = "KD-Tree";
        } else
#endif
        if (simpleColorDB.isOpen()) {
          searchMethod = "Binary DB";
        }
        
        Logger::debug("Color lookup: RGB(" + String(currentR) + "," + String(currentG) + "," + String(currentB) + 
                     ") -> " + colorName + " | Search: " + String(colorSearchTime) + "Î¼s (" + searchMethod + ")");
      }
    }
  }

  // Print result information less frequently for better performance
  static unsigned long lastLogTime = 0;
  static unsigned long lastPerfCheck = 0;
  
  if (millis() - lastLogTime > 5000) { // Log every 5 seconds to reduce overhead
    Logger::info("XYZ: " + String(XData) + "," + String(YData) + "," + String(ZData) +
                 " | RGB: R" + String(smoothed_R, 2) + " G" + String(smoothed_G, 2) + " B" + String(smoothed_B, 2) +
                 " | Color: " + currentColorData.colorName + " | Last search: " + String(currentColorData.colorSearchDuration) + "Î¼s");
    lastLogTime = millis();
  }
  
  // Periodic performance monitoring (every 30 seconds)
  if (millis() - lastPerfCheck > 30000) {
    size_t currentFreeHeap = ESP.getFreeHeap();
    size_t currentFreePsram = psramFound() ? ESP.getFreePsram() : 0;
    
    // Check for memory leaks or degradation
    static size_t lastFreeHeap = currentFreeHeap;
    static size_t lastFreePsram = currentFreePsram;
    
    if (settings.debugMemoryUsage) {
      Logger::debug("Performance Monitor: Heap=" + String(currentFreeHeap / 1024) + "KB, PSRAM=" + String(currentFreePsram / 1024) + "KB");
      
      if (lastFreeHeap > 0 && currentFreeHeap < lastFreeHeap - 10000) {
        Logger::warn("Heap memory decrease detected: " + String((lastFreeHeap - currentFreeHeap) / 1024) + "KB");
      }
      
      if (lastFreePsram > 0 && currentFreePsram < lastFreePsram - 100000) {
        Logger::warn("PSRAM decrease detected: " + String((lastFreePsram - currentFreePsram) / 1024) + "KB");
      }
    }
    
    // Performance optimization: Check if we need to adjust search method
    if (currentFreePsram < (PSRAM_SAFETY_MARGIN_KB * 1024) && settings.enableKdtree) {
      Logger::warn("PSRAM low - disabling KD-tree to conserve memory");
      settings.enableKdtree = false;
    }
    
    lastFreeHeap = currentFreeHeap;
    lastFreePsram = currentFreePsram;
    lastPerfCheck = millis();
  }
}

// Simplified individual setting handlers using GET requests for reliability
void handleSetColorSamples(AsyncWebServerRequest *request) {
  if (request->hasParam("value")) {
    int samples = request->getParam("value")->value().toInt();
    if (samples >= 1 && samples <= 10) {
      settings.colorReadingSamples = samples;
      Logger::info("Color samples updated to: " + String(samples));
      request->send(200, "application/json", "{\"status\":\"success\",\"colorSamples\":" + String(samples) + "}");
    } else {
      request->send(400, "application/json", "{\"error\":\"Color samples must be 1-10\"}");
    }
  } else {
    request->send(400, "application/json", "{\"error\":\"Missing value parameter\"}");
  }
}

void handleSetSampleDelay(AsyncWebServerRequest *request) {
  if (request->hasParam("value")) {
    int delay = request->getParam("value")->value().toInt();
    if (delay >= 1 && delay <= 50) {
      settings.sensorSampleDelay = delay;
      Logger::info("Sample delay updated to: " + String(delay) + "ms");
      request->send(200, "application/json", "{\"status\":\"success\",\"sampleDelay\":" + String(delay) + "}");
    } else {
      request->send(400, "application/json", "{\"error\":\"Sample delay must be 1-50ms\"}");
    }
  } else {
    request->send(400, "application/json", "{\"error\":\"Missing value parameter\"}");
  }
}

void handleSetDebugSettings(AsyncWebServerRequest *request) {
  bool updated = false;
  String response = "{\"status\":\"success\"";
  
  if (request->hasParam("sensor")) {
    bool enable = request->getParam("sensor")->value() == "true";
    settings.debugSensorReadings = enable;
    response += ",\"debugSensor\":" + String(enable ? "true" : "false");
    updated = true;
  }
  
  if (request->hasParam("colors")) {
    bool enable = request->getParam("colors")->value() == "true";
    settings.debugColorMatching = enable;
    response += ",\"debugColors\":" + String(enable ? "true" : "false");
    updated = true;
  }
  
  response += "}";

  // Immediate response for debug settings
  request->send(200, "application/json", response);

  // Log updated settings immediately
  Logger::info("Debug settings updated: " + response);
}

// Calibration API handlers

// GET /api/calibration - Returns current calibration coefficients
void handleGetCalibration(AsyncWebServerRequest *request) {
  PsramAllocator allocator;
  JsonDocument doc(&allocator);
  
  doc["useDFRobotLibraryCalibration"] = settings.useDFRobotLibraryCalibration;
  doc["calibrationMode"] = settings.useDFRobotLibraryCalibration ? "dfrobot" : "custom";
  
  doc["redA"] = settings.redA;
  doc["redB"] = settings.redB;
  doc["redC"] = settings.redC;
  doc["greenA"] = settings.greenA;
  doc["greenB"] = settings.greenB;
  doc["greenC"] = settings.greenC;
  doc["blueA"] = settings.blueA;
  doc["blueB"] = settings.blueB;
  doc["blueC"] = settings.blueC;
  // White coefficients
  doc["whiteRedA"] = settings.whiteRedA;
  doc["whiteRedB"] = settings.whiteRedB;
  doc["whiteRedC"] = settings.whiteRedC;
  doc["whiteGreenA"] = settings.whiteGreenA;
  doc["whiteGreenB"] = settings.whiteGreenB;
  doc["whiteGreenC"] = settings.whiteGreenC;
  doc["whiteBlueA"] = settings.whiteBlueA;
  doc["whiteBlueB"] = settings.whiteBlueB;
  doc["whiteBlueC"] = settings.whiteBlueC;
  // Grey coefficients
  doc["greyRedA"] = settings.greyRedA;
  doc["greyRedB"] = settings.greyRedB;
  doc["greyRedC"] = settings.greyRedC;
  doc["greyGreenA"] = settings.greyGreenA;
  doc["greyGreenB"] = settings.greyGreenB;
  doc["greyGreenC"] = settings.greyGreenC;
  doc["greyBlueA"] = settings.greyBlueA;
  doc["greyBlueB"] = settings.greyBlueB;
  doc["greyBlueC"] = settings.greyBlueC;
  // Dynamic settings
  doc["enableDynamicCalibration"] = settings.enableDynamicCalibration;
  doc["dynamicThreshold"] = settings.dynamicThreshold;
  
  String response;
  serializeJson(doc, response);
  
  AsyncWebServerResponse *apiResponse = request->beginResponse(200, "application/json", response);
  apiResponse->addHeader("Access-Control-Allow-Origin", "*");
  request->send(apiResponse);
  
  Logger::info("Calibration settings sent to client");
}

// POST /api/tune-vivid-white - Optimizes calibration for Vivid White (247,248,244)
void handleTuneVividWhite(AsyncWebServerRequest *request) {
  Logger::info("Tuning calibration for Vivid White (247,248,244)");
  
  // Apply optimized coefficients for Vivid White target (significantly reduced)
  // These values are tuned for a more balanced, moderate white output to avoid saturation
  settings.redA = 4.856615248518086e-06f;   // Reduced from 5.856 for less bright red
  settings.redB = -0.09624971353127427f;    // Reduced from -0.106 for gentler linearity
  settings.redC = 580.2283515839658f;       // Significantly reduced from 668 for much lower brightness
  
  settings.greenA = 6.800364703908128e-06f; // Reduced from 7.800 for more balanced green
  settings.greenB = -0.13773455804115546f;  // Reduced from -0.147 for gentler adjustment
  settings.greenC = 720.288778468652f;      // Significantly reduced from 860 for lower brightness
  
  settings.blueA = -2.2588632792769936e-06f; // Less negative for more balanced blue
  settings.blueB = 0.04159423885676833f;     // Reduced from 0.051 for gentler blue
  settings.blueC = 28.55576869603341f;       // Reduced from 38 for lower baseline
  
  Logger::info("Vivid White calibration applied");
  Logger::info("New Red: A=" + String(settings.redA, 10) + " B=" + String(settings.redB, 6) + " C=" + String(settings.redC, 2));
  Logger::info("New Green: A=" + String(settings.greenA, 10) + " B=" + String(settings.greenB, 6) + " C=" + String(settings.greenC, 2));
  Logger::info("New Blue: A=" + String(settings.blueA, 10) + " B=" + String(settings.blueB, 6) + " C=" + String(settings.blueC, 2));
  
  // Return the new calibration values
  PsramAllocator allocator;
  JsonDocument doc(&allocator);
  
  doc["status"] = "success";
  doc["message"] = "Tuned for Vivid White (247,248,244)";
  doc["calibration"]["redA"] = settings.redA;
  doc["calibration"]["redB"] = settings.redB;
  doc["calibration"]["redC"] = settings.redC;
  doc["calibration"]["greenA"] = settings.greenA;
  doc["calibration"]["greenB"] = settings.greenB;
  doc["calibration"]["greenC"] = settings.greenC;
  doc["calibration"]["blueA"] = settings.blueA;
  doc["calibration"]["blueB"] = settings.blueB;
  doc["calibration"]["blueC"] = settings.blueC;
  
  String response;
  serializeJson(doc, response);
  
  AsyncWebServerResponse *apiResponse = request->beginResponse(200, "application/json", response);
  apiResponse->addHeader("Access-Control-Allow-Origin", "*");
  request->send(apiResponse);
}

// === GREY CALIBRATION AUTO-TUNED FOR TARGET RGB(168,160,147) ===
// Target grey port RGB values: R=168, G=160, B=147
// Previous readings: RGB(118,113,54) -> needed 42% increase for R/G, 172% for B
// Auto-tuned dark matrix coefficients to achieve precise target values
// Key settings for target calibration:
// - dynamicThreshold: 8000.0f (determines when to use dark vs bright matrix)
// - darkMatrix: Tuned for target RGB(168,160,147)
// - IR compensation: Optimized at 0.20 for accuracy
// - RGB_SATURATION_LIMIT: 255 (allows higher values)

