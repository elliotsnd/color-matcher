/*!
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

// 🎛️ EASY SETTINGS - All adjustable parameters are in sensor_settings.h
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

DFRobot_TCS3430 TCS3430;

// I2C pin definitions for ESP32-S3 ProS3
#define SDA_PIN 3
#define SCL_PIN 4

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
  
  // Color Calibration Settings
  float irCompensationFactor1 = IR_COMPENSATION_FACTOR_1;
  float irCompensationFactor2 = IR_COMPENSATION_FACTOR_2;
  uint8_t rgbSaturationLimit = RGB_SATURATION_LIMIT;
  
  // Calibration Parameters
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
  Logger::info("Free heap before loading: " + String(esp_get_free_heap_size()));
  if (psramFound()) {
    Logger::info("Free PSRAM before loading: " + String(ESP.getFreePsram() / 1024) + " KB");
  } else {
    Logger::error("PSRAM not available - using fallback color database");
    return loadFallbackColors();
  }

  Logger::info("Memory check complete, proceeding with binary file loading...");

  // Try to open binary database first (preferred method)
  Logger::info("Attempting to open binary color database: /dulux.bin");
  if (simpleColorDB.openDatabase("/dulux.bin")) {
    unsigned long loadTime = millis() - startTime;
    Logger::info("Binary color database opened successfully!");
    Logger::info("Colors available: " + String(simpleColorDB.getColorCount()));
    Logger::info("Open time: " + String(loadTime) + "ms");
    Logger::info("PSRAM free after open: " + String(ESP.getFreePsram() / 1024) + " KB");
    
#if ENABLE_KDTREE
    // Initialize KD-tree with data from binary database
    Logger::info("Building lightweight KD-tree for optimized color search...");
    unsigned long kdStartTime = millis();
    
    size_t colorCount = simpleColorDB.getColorCount();
    Logger::info("Loading " + String(colorCount) + " colors into KD-tree...");
    
    // Safety check for large datasets
    if (colorCount > 10000) {
      Logger::warn("Large color dataset detected (" + String(colorCount) + " colors)");
      Logger::warn("This may take significant time and memory");
    }
    
    // Create vector of color points for the lightweight KD-tree
    PSRAMColorVector colorPoints;
    colorPoints.reserve(colorCount);
    
    // Load colors with progress logging and timeout protection
    size_t loadedCount = 0;
    unsigned long loadStartTime = millis();
    const unsigned long maxLoadTime = KDTREE_LOAD_TIMEOUT_MS; // Configurable timeout for loading
    
    for (size_t i = 0; i < colorCount; i++) {
      // Check for timeout during loading
      if (millis() - loadStartTime > maxLoadTime) {
        Logger::warn("Color loading timeout after " + String((millis() - loadStartTime) / 1000) + " seconds");
        Logger::warn("Loaded " + String(loadedCount) + " of " + String(colorCount) + " colors before timeout");
        break;
      }
      
      SimpleColor color;
      if (simpleColorDB.getColorByIndex(i, color)) {
        ColorPoint point(color.r, color.g, color.b, (uint16_t)i);
        colorPoints.push_back(point);
        loadedCount++;
        
        // Progress logging for large datasets with memory monitoring
        if (colorCount > 1000 && (i % 500 == 0 || i == colorCount - 1)) {
          Logger::info("Loaded " + String(i + 1) + "/" + String(colorCount) + " colors");
          
          // Check available memory and performance
          size_t freeHeap = ESP.getFreeHeap();
          size_t freePsram = ESP.getFreePsram();
          unsigned long elapsedTime = millis() - loadStartTime;
          
          Logger::info("Memory: Heap=" + String(freeHeap) + ", PSRAM=" + String(freePsram) + ", Time=" + String(elapsedTime) + "ms");
          
          if (freeHeap < 50000 || freePsram < 500000) {
            Logger::warn("Low memory during color loading - may need to limit dataset");
            // Consider breaking early if memory is very low
            if (freeHeap < 30000) {
              Logger::error("Critical memory low - stopping color loading");
              break;
            }
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
    
    Logger::info("Successfully loaded " + String(loadedCount) + " colors");
    
    if (loadedCount == 0) {
      Logger::error("No colors loaded - skipping KD-tree construction");
    } else {
      Logger::info("Starting lightweight KD-tree construction...");
      Logger::info("Free memory before KD-tree: Heap=" + String(ESP.getFreeHeap()) + ", PSRAM=" + String(ESP.getFreePsram()));
      
      if (kdTreeColorDB.build(colorPoints)) {
        unsigned long kdLoadTime = millis() - kdStartTime;
        Logger::info("Lightweight KD-tree built successfully in " + String(kdLoadTime) + "ms");
        Logger::info("KD-tree nodes: " + String(kdTreeColorDB.getNodeCount()));
        Logger::info("KD-tree memory usage: " + String(kdTreeColorDB.getMemoryUsage()) + " bytes");
        Logger::info("PSRAM free after KD-tree: " + String(ESP.getFreePsram() / 1024) + " KB");
      } else {
        Logger::error("Failed to build KD-tree - falling back to binary database only");
      }
    }
#else
    Logger::info("KD-tree disabled - using binary database only");
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
  Logger::debug("Finding closest color for RGB(" + String(r) + "," + String(g) + "," + String(b) + ")");

#if ENABLE_KDTREE
  // Try KD-tree search first (fastest - O(log n) average case)
  if (kdTreeColorDB.isBuilt()) {
    ColorPoint closest = kdTreeColorDB.findNearest(r, g, b);
    if (closest.index > 0) {
      // Get the full color data using the index
      SimpleColor color;
      if (simpleColorDB.getColorByIndex(closest.index, color)) {
        String bestMatch = String(color.name) + " (" + String(color.code) + ")";
        Logger::debug("KD-tree color matching completed. Best match: " + bestMatch);
        return bestMatch;
      }
    }
    Logger::warn("KD-tree search failed, falling back to binary database");
  }
#endif

  // Fallback to simple binary database with optimized search
  SimpleColor closestColor;
  if (simpleColorDB.findClosestColor(r, g, b, closestColor)) {
    String bestMatch = String(closestColor.name) + " (" + String(closestColor.code) + ")";
    Logger::debug("Binary color matching completed. Best match: " + bestMatch);
    return bestMatch;
  }

  // Fallback to legacy database if available
  if (fallbackColorDatabase != nullptr && fallbackColorCount > 0) {
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
          Logger::debug("Perfect match found: " + bestMatch);
          break;
        }
      }
    }

    Logger::debug("Fallback color matching completed. Best match: " + bestMatch + " (distance: " + String(minDistance) + ")");
    return bestMatch;
  }

  // Final fallback to basic color names
  Logger::warn("No color database available, using basic color classification");
  if (r > 200 && g > 200 && b > 200) return "Light Color";
  if (r < 50 && g < 50 && b < 50) return "Dark Color";
  if (r > g && r > b) return "Red Tone";
  if (g > r && g > b) return "Green Tone";
  if (b > r && b > g) return "Blue Tone";
  return "Mixed Color";
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
struct ColorData {
  uint16_t x, y, z, ir1, ir2;
  uint8_t r, g, b;  // Integer values for web interface
  float r_precise, g_precise, b_precise;  // Float values for precision logging
  String colorName;
  unsigned long timestamp;
} currentColorData;

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
  doc["r"] = currentColorData.r;
  doc["g"] = currentColorData.g;
  doc["b"] = currentColorData.b;
  doc["x"] = currentColorData.x;
  doc["y"] = currentColorData.y;
  doc["z"] = currentColorData.z;
  doc["ir1"] = currentColorData.ir1;
  doc["ir2"] = currentColorData.ir2;
  doc["colorName"] = currentColorData.colorName;
  doc["timestamp"] = currentColorData.timestamp;

  String response;
  serializeJson(doc, response);
  Logger::debug("JSON response size: " + String(response.length()));
  
  // Add CORS headers for local development
  AsyncWebServerResponse *apiResponse = request->beginResponse(200, "application/json", response);
  apiResponse->addHeader("Access-Control-Allow-Origin", "*");
  request->send(apiResponse);
  Logger::debug("Color API response sent successfully");
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

// This function applies the final calibration with optimized quadratic precision
void convertXYZtoRGB_Calibrated(uint16_t X, uint16_t Y, uint16_t Z, uint16_t IR1, uint16_t IR2, uint8_t &R, uint8_t &G, uint8_t &B) {
  if (settings.debugSensorReadings) {
    Serial.print("[DEBUG] Converting XYZ to RGB - X:");
    Serial.print(X); Serial.print(" Y:"); Serial.print(Y); Serial.print(" Z:"); Serial.print(Z); 
    Serial.print(" IR1:"); Serial.print(IR1); Serial.print(" IR2:"); Serial.println(IR2);
  }
  
  // Apply configurable IR compensation using runtime settings
  float X_adj = X - (settings.irCompensationFactor1 * IR1) - (settings.irCompensationFactor2 * IR2);
  float Y_adj = Y - (settings.irCompensationFactor1 * IR1) - (settings.irCompensationFactor2 * IR2);
  float Z_adj = Z - (settings.irCompensationFactor1 * IR1) - (settings.irCompensationFactor2 * IR2);

  if (settings.debugSensorReadings) {
    Serial.print("[DEBUG] After IR compensation - X_adj:");
    Serial.print(X_adj); Serial.print(" Y_adj:"); Serial.print(Y_adj); Serial.print(" Z_adj:"); Serial.println(Z_adj);
    Serial.print("[DEBUG] IR compensation factors - IR1_factor:");
    Serial.print(settings.irCompensationFactor1); Serial.print(" IR2_factor:"); Serial.println(settings.irCompensationFactor2);
  }

  // Latest optimized quadratic parameters (balanced for all three targets)
  const float A_R = 5.756615248518086e-06f, B_R = -0.10824971353127427f, C_R = 663.2283515839658f;  // Increased by 3 for Vivid White compromise
  const float A_G = 7.700364703908128e-06f, B_G = -0.14873455804115546f, C_G = 855.288778468652f;   // Increased by 3 for Vivid White compromise  
  const float A_B = -2.7588632792769936e-06f, B_B = 0.04959423885676833f, C_B = 35.55576869603341f; // Increased by 1.5 for Vivid White compromise

  // Calculate each channel
  float r_final = A_R * X_adj * X_adj + B_R * X_adj + C_R;
  float g_final = A_G * Y_adj * Y_adj + B_G * Y_adj + C_G;
  float b_final = A_B * Z_adj * Z_adj + B_B * Z_adj + C_B;

  if (settings.debugSensorReadings) {
    Serial.print("[DEBUG] Raw RGB calculations - r:");
    Serial.print(r_final); Serial.print(" g:"); Serial.print(g_final); Serial.print(" b:"); Serial.println(b_final);
  }

  // Clamp to 0-RGB_SATURATION_LIMIT using configurable limit
  R = (uint8_t)max(0.0f, min((float)settings.rgbSaturationLimit, r_final));
  G = (uint8_t)max(0.0f, min((float)settings.rgbSaturationLimit, g_final));
  B = (uint8_t)max(0.0f, min((float)settings.rgbSaturationLimit, b_final));
  
  if (settings.debugSensorReadings) {
    Serial.print("[DEBUG] Final clamped RGB - R:");
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
  TCS3430.setALSGain(3);
  Logger::debug("ALS gain set to 3");

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

  uint8_t R, G, B;

  // Convert to RGB using quadratic calibration with IR1 and IR2 compensation
  convertXYZtoRGB_Calibrated(XData, YData, ZData, IR1Data, IR2Data, R, G, B);

  // Lightweight smoothing filter for responsive color changes
  static float smoothed_R = R, smoothed_G = G, smoothed_B = B;
  const float smoothing_factor = 0.1f;  // Reduced from 0.3 for maximum responsiveness
  smoothed_R = smoothed_R * smoothing_factor + R * (1.0f - smoothing_factor);
  smoothed_G = smoothed_G * smoothing_factor + G * (1.0f - smoothing_factor);
  smoothed_B = smoothed_B * smoothing_factor + B * (1.0f - smoothing_factor);

  // Find closest Dulux color name (using rounded smoothed values) with timing
  unsigned long colorSearchStart = micros();
  String colorName = findClosestDuluxColor((uint8_t)round(smoothed_R), (uint8_t)round(smoothed_G), (uint8_t)round(smoothed_B));
  unsigned long colorSearchTime = micros() - colorSearchStart;

  // Update current color data for API (use smoothed values)
  currentColorData = {XData, YData, ZData, IR1Data, IR2Data,
                      (uint8_t)round(smoothed_R), (uint8_t)round(smoothed_G), (uint8_t)round(smoothed_B),  // Integer values for web
                      smoothed_R, smoothed_G, smoothed_B,  // Float values for precision
                      colorName, millis()};

  // Print result information less frequently for better performance
  static unsigned long lastLogTime = 0;
  if (millis() - lastLogTime > 5000) { // Log every 5 seconds to reduce overhead
    String searchMethod = "Fallback";
#if ENABLE_KDTREE
    if (kdTreeColorDB.isBuilt()) {
      searchMethod = "KD-Tree";
    } else
#endif
    if (simpleColorDB.isOpen()) {
      searchMethod = "Binary DB";
    }
    Logger::info("XYZ: " + String(XData) + "," + String(YData) + "," + String(ZData) +
                 " | RGB: R" + String(smoothed_R, 2) + " G" + String(smoothed_G, 2) + " B" + String(smoothed_B, 2) +
                 " | Color: " + colorName + " | Search: " + String(colorSearchTime) + "μs (" + searchMethod + ")");
    lastLogTime = millis();
  }

  delay(100); 
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