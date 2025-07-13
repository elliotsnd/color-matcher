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
#include <DFRobot_TCS3430.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <Wire.h>
#include <esp_heap_caps.h>
#include "dulux_simple_reader.h"

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
LogLevel Logger::currentLevel = LOG_INFO; // Default to INFO level

// WiFi credentials
const char* ssid = "Wifi 6";
const char* password = "Scrofani1985";

// AP mode credentials
const char* ap_ssid = "colormatcher";
const char* ap_password = "Scrofani1985";

// Static IP configuration
IPAddress local_IP(192, 168, 0, 152);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

// Web server on port 80
AsyncWebServer server(80);

DFRobot_TCS3430 TCS3430;

// I2C pin definitions for ESP32-S3 ProS3
#define SDA_PIN 3
#define SCL_PIN 4

// Set the optimized LED brightness
int LEDpin = 5;
int ledBrightness = 100; // Final optimized brightness

const uint16_t SATURATION_THRESHOLD = 65000;

// === START OF FINAL, DEFINITIVE CALIBRATION PARAMETERS ===
// Confirmed to produce accurate results for both targets.

// IR Compensation Factor
const float IR_COMPENSATION = 0.5f;

// Red Channel Calibration (maps from X)
const float R_SLOPE = 0.01180f;
const float R_OFFSET = 52.28f;

// Green Channel Calibration (maps from Y)
const float G_SLOPE = 0.01359f;
const float G_OFFSET = 24.06f;

// Blue Channel Calibration (maps from Z)
const float B_SLOPE = 0.01904f;
const float B_OFFSET = 79.37f;

// === END OF CALIBRATION PARAMETERS ===

// Simple binary color database reader
DuluxSimpleReader simpleColorDB;

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
// Calculate color distance using weighted Euclidean distance
float calculateColorDistance(uint8_t r1, uint8_t g1, uint8_t b1, uint8_t r2, uint8_t g2, uint8_t b2) {
  // Weighted RGB distance - human eye is more sensitive to green
  float dr = (r1 - r2) * 0.30f;
  float dg = (g1 - g2) * 0.59f; 
  float db = (b1 - b2) * 0.11f;
  return sqrt(dr*dr + dg*dg + db*db);
}

// Find the closest Dulux color match using binary database
String findClosestDuluxColor(uint8_t r, uint8_t g, uint8_t b) {
  Logger::debug("Finding closest color for RGB(" + String(r) + "," + String(g) + "," + String(b) + ")");

  // Try simple binary database first
  SimpleColor closestColor;
  if (simpleColorDB.findClosestColor(r, g, b, closestColor)) {
    String bestMatch = String(closestColor.name) + " (" + String(closestColor.code) + ")";
    Logger::debug("Binary color matching completed. Best match: " + bestMatch);
    return bestMatch;
  }

  // Fallback to legacy database if available
  if (fallbackColorDatabase != nullptr && fallbackColorCount > 0) {
    Logger::debug("Using fallback color database with " + String(fallbackColorCount) + " colors...");

    float minDistance = 999999.0f;
    String bestMatch = "Unknown";
    int bestIndex = -1;

    // Search through fallback color database
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
  uint8_t r, g, b;
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

// This function applies the final calibration
void convertXYZtoRGB_Calibrated(uint16_t X, uint16_t Y, uint16_t Z, uint16_t IR1, uint8_t &R, uint8_t &G, uint8_t &B) {
  Serial.print("[DEBUG] Converting XYZ to RGB - X:");
  Serial.print(X); Serial.print(" Y:"); Serial.print(Y); Serial.print(" Z:"); Serial.print(Z); Serial.print(" IR1:"); Serial.println(IR1);
  
  // Apply IR compensation
  float X_adj = X - IR_COMPENSATION * IR1;
  float Y_adj = Y - IR_COMPENSATION * IR1;
  float Z_adj = Z - IR_COMPENSATION * IR1;

  Serial.print("[DEBUG] After IR compensation - X_adj:");
  Serial.print(X_adj); Serial.print(" Y_adj:"); Serial.print(Y_adj); Serial.print(" Z_adj:"); Serial.println(Z_adj);

  // Calculate each channel using its unique linear formula
  float r_final = R_SLOPE * X_adj + R_OFFSET;
  float g_final = G_SLOPE * Y_adj + G_OFFSET;
  float b_final = B_SLOPE * Z_adj + B_OFFSET;

  Serial.print("[DEBUG] Raw RGB calculations - r:");
  Serial.print(r_final); Serial.print(" g:"); Serial.print(g_final); Serial.print(" b:"); Serial.println(b_final);

  // Clamp the values to the valid 0-255 range and assign to the output
  R = (uint8_t)max(0.0f, min(255.0f, r_final));
  G = (uint8_t)max(0.0f, min(255.0f, g_final));
  B = (uint8_t)max(0.0f, min(255.0f, b_final));
  
  Serial.print("[DEBUG] Final clamped RGB - R:");
  Serial.print(R); Serial.print(" G:"); Serial.print(G); Serial.print(" B:"); Serial.println(B);
}

// Uncalibrated function for comparison
void convertXYZtoRGB_Uncalibrated(uint16_t X, uint16_t Y, uint16_t Z, uint8_t &R, uint8_t &G, uint8_t &B) {
    float x = X / 65535.0f; float y = Y / 65535.0f; float z = Z / 65535.0f;
    float r_linear = 3.2406f*x-1.5372f*y-0.4986f*z; float g_linear=-0.9689f*x+1.8758f*y+0.0415f*z; float b_linear=0.0557f*x-0.2040f*y+1.0570f*z;
    r_linear=max(0.0f,min(1.0f,r_linear)); g_linear=max(0.0f,min(1.0f,g_linear)); b_linear=max(0.0f,min(1.0f,b_linear));
    float gamma=1.0f/2.2f; R=(uint8_t)(pow(r_linear,gamma)*255.0f); G=(uint8_t)(pow(g_linear,gamma)*255.0f); B=(uint8_t)(pow(b_linear,gamma)*255.0f);
}

void setup() {
  Serial.begin(115200);
  Logger::info("System startup initiated - Serial communication started at 115200 baud");

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

  // Initialize I2C with custom pins for ESP32-S3 ProS3
  Wire.begin(SDA_PIN, SCL_PIN);
  Logger::debug("I2C initialized with SDA=3, SCL=4");

  pinMode(LEDpin, OUTPUT);
  // Set the final, lower brightness
  analogWrite(LEDpin, ledBrightness);
  Logger::debug("LED pin configured, brightness set to: " + String(ledBrightness));

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
  TCS3430.setIntegrationTime(0x23);
  Logger::debug("Integration time set to 0x23");
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

  // Configure static IP
  Logger::debug("Configuring static IP address...");
  if (!WiFi.config(local_IP, gateway, subnet)) {
    Logger::error("Static IP configuration failed, using DHCP");
  } else {
    Logger::debug("Static IP configuration successful");
  }

  // Try to connect to WiFi
  Logger::info("Starting WiFi connection to SSID: " + String(ssid));
  WiFi.begin(ssid, password);
  
  unsigned long wifiStartTime = millis();
  int retryCount = 0;
  
  // Keep trying to connect until successful
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    
    // Restart WiFi connection attempt every 30 seconds if still not connected
    static unsigned long lastAttempt = 0;
    if (millis() - lastAttempt > 30000) {
      retryCount++;
      Serial.println();
      Logger::debug("WiFi connection attempt #" + String(retryCount) + " failed, retrying...");
      WiFi.disconnect();
      Logger::debug("WiFi disconnected");
      delay(1000);
      WiFi.begin(ssid, password);
      Logger::debug("WiFi reconnection initiated");
      lastAttempt = millis();
    }
  }
  
  Serial.println();
  Logger::info("WiFi connected successfully!");
  Logger::info("IP address: " + WiFi.localIP().toString());
  Logger::info("Total connection time: " + String((millis() - wifiStartTime) / 1000) + " seconds");

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

void loop() {
  // AsyncWebServer handles requests automatically, no need for handleClient()
  
  // Read sensor data (simplified logging)
  uint16_t XData = TCS3430.getXData();
  uint16_t YData = TCS3430.getYData();
  uint16_t ZData = TCS3430.getZData();
  uint16_t IR1Data = TCS3430.getIR1Data();
  uint16_t IR2Data = TCS3430.getIR2Data();

  uint8_t R, G, B;
  
  // Convert to RGB using calibration
  convertXYZtoRGB_Calibrated(XData, YData, ZData, IR1Data, R, G, B);

  // Find closest Dulux color name
  String colorName = findClosestDuluxColor(R, G, B);

  // Update current color data for API
  currentColorData = {XData, YData, ZData, IR1Data, IR2Data, R, G, B, colorName, millis()};

  // Print only essential result information
  Logger::info("XYZ: " + String(XData) + "," + String(YData) + "," + String(ZData) + 
               " | RGB: " + String(R) + "," + String(G) + "," + String(B) + 
               " | Color: " + colorName);

  delay(1000); // Slower update rate for cleaner output
}