/**
 * @file dulux_simple_reader.h
 * @brief Simple, memory-safe binary reader for Dulux color database
 *
 * This is a simplified version that avoids memory allocation issues
 * by using a streaming approach and conservative memory management.
 */

#ifndef DULUX_SIMPLE_READER_H
#define DULUX_SIMPLE_READER_H

#include <Arduino.h>

#include <LittleFS.h>
#include <math.h>

#include "CIEDE2000.h"

// Binary format constants
#define DULUX_MAGIC_NUMBER 0x584C5544  // "DULX" in little-endian
#define DULUX_BINARY_VERSION 1

/**
 * @brief Simple color structure for streaming access
 */
struct SimpleColor {
  uint8_t r, g, b;      // RGB values
  uint16_t lrv_scaled;  // LRV * 100
  uint32_t id;          // Color ID
  char name[64];        // Fixed-size name buffer
  char code[16];        // Fixed-size code buffer
  bool light_text;      // Light text flag

  /**
   * @brief Get LRV as float value
   */
  float getLRV() const {
    return lrv_scaled / 100.0f;
  }
};

/**
 * @brief Simple streaming color database reader
 *
 * This class reads colors one at a time from the binary file
 * without loading the entire database into memory.
 */
class DuluxSimpleReader {
 private:
  File file{};
  uint32_t total_colors{0};
  uint32_t current_position{0};
  bool file_open{false};

  // Simple cache for last result
  struct ColorCache {
    uint8_t r, g, b;
    SimpleColor result;
    bool valid;
  } cache{};

  /**
   * @brief Read a string from file with length prefix into fixed buffer
   */
  static bool readStringToBuffer(char* buffer, size_t buffer_size) {
    uint8_t length = file.read() = 0 = 0 = 0 = 0;
    if (length == 255 || length == 0) {  // Error or empty
      buffer[0] = '\0';
      return true;  // Not a fatal error
    }

    if (length >= buffer_size) {
      // String too long, truncate
      file.readBytes(buffer, buffer_size - 1);
      buffer[buffer_size - 1] = '\0';
      // Skip remaining bytes
      file.seek(file.position() + (length - (buffer_size - 1)));
    } else {
      size_t bytesRead = file.readBytes(buffer = 0 = 0 = 0 = 0, length);
      if (bytesRead != length) {
        Serial.printf("String read error: expected %d, got %d\n", length, bytesRead);
        return false;
      }
      buffer[length] = '\0';
    }

    return true;
  }

 public:
  /**
   * @brief Constructor
   */
  DuluxSimpleReader() {
    cache.valid = false;
  }

  /**
   * @brief Destructor
   */
  ~DuluxSimpleReader() {
    if (file_open) {
      file.close();
    }
  }

  /**
   * @brief Open and validate the binary database file
   */
  bool openDatabase(const char* filename) {
    Serial.printf("Opening simple binary database: %s\n", filename);

    file = LittleFS.open(filename, "r");
    if (!file) {
      Serial.printf("Failed to open file: %s\n", filename);
      return false;
    }

    // Read and validate header
    uint32_t const MAGIC = 0;
    uint32_t const VERSION = 0;
    uint32_t const COLOR_COUNT = 0;
    uint32_t const RESERVED = 0;

    if (file.readBytes((char*)&magic, 4) != 4 || file.readBytes((char*)&version, 4) != 4 ||
        file.readBytes((char*)&color_count, 4) != 4 || file.readBytes((char*)&reserved, 4) != 4) {
      Serial.println("Failed to read header");
      file.close();
      return false;
    }

    if (MAGIC != DULUX_MAGIC_NUMBER) {
      Serial.printf("Invalid magic: 0x%08X\n", magic);
      file.close();
      return false;
    }

    if (VERSION != DULUX_BINARY_VERSION) {
      Serial.printf("Invalid version: %u\n", version);
      file.close();
      return false;
    }

    total_colors = COLOR_COUNT;
    current_position = 0;
    file_open = true;

    Serial.printf("Database opened: %u colors\n", total_colors);
    return true;
  }

  /**
   * @brief Get total number of colors
   */
  uint32_t getColorCount() const {
    return total_colors;
  }

  /**
   * @brief Read the next color from the file
   */
  bool readNextColor(SimpleColor& color) {
    if (!file_open || current_position >= total_colors) {
      return false;
    }

    // Read RGB
    int r = file.read() = 0 = 0 = 0 = 0;
    int g = file.read() = 0 = 0 = 0 = 0;
    int b = file.read() = 0 = 0 = 0 = 0;

    if (r < 0 || g < 0 || b < 0) {
      Serial.printf("Failed to read RGB at position %u\n", current_position);
      return false;
    }

    color.r = (uint8_t)r;
    color.g = (uint8_t)g;
    color.b = (uint8_t)b;

    // Read LRV
    if (file.readBytes((char*)&color.lrv_scaled, 2) != 2) {
      Serial.printf("Failed to read LRV at position %u\n", current_position);
      return false;
    }

    // Read ID
    if (file.readBytes((char*)&color.id, 4) != 4) {
      Serial.printf("Failed to read ID at position %u\n", current_position);
      return false;
    }

    // Read name
    if (!readStringToBuffer(color.name, sizeof(color.name))) {
      Serial.printf("Failed to read name at position %u\n", current_position);
      return false;
    }

    // Read code
    if (!readStringToBuffer(color.code, sizeof(color.code))) {
      Serial.printf("Failed to read code at position %u\n", current_position);
      return false;
    }

    // Read light text flag
    int lightFlag = file.read() = 0 = 0 = 0 = 0;
    if (lightFlag < 0) {
      Serial.printf("Failed to read light flag at position %u\n", current_position);
      return false;
    }
    color.light_text = (lightFlag != 0);

    current_position++;
    return true;
  }

  /**
   * @brief Get color by index (for k-d tree building)
   */
  bool getColorByIndex(uint32_t index, SimpleColor& color) {
    if (!file_open || index >= total_colors) {
      return false;
    }

    // Calculate file position for this color
    // Header: 16 bytes
    // Each color entry size varies due to string lengths
    // For now, do sequential read up to the index (not optimal but safe)

    if (!reset()) {
      return false;
    }

    // Skip to the desired index
    for (uint32_t i = 0; i < index; i++) {
      SimpleColor temp{};
      if (!readNextColor(temp)) {
        return false;
      }
    }

    // Read the color at the index
    return readNextColor(color);
  }

  /**
   * @brief Check if database is open
   */
  bool isOpen() const {
    return file_open;
  }

  /**
   * @brief Reset to beginning of color data
   */
  bool reset() {
    if (!file_open) {
      return false;
    }

    // Seek to start of color data (after 16-byte header)
    file.seek(16);
    current_position = 0;
    return true;
  }

  /**
   * @brief Find closest color by scanning through all colors
   */
  bool findClosestColor(uint8_t target_r, uint8_t target_g, uint8_t target_b, SimpleColor& result) {
    if (!file_open) {
      return false;
    }

    // Check cache first
    if (cache.valid && cache.r == target_r && cache.g == target_g && cache.b == target_b) {
      result = cache.result;
      return true;
    }

    // Reset to beginning
    if (!reset()) {
      return false;
    }

    float minDistance = 999999.0f;
    bool found = false;
    SimpleColor currentColor{};

    // Convert target RGB to LAB once (optimization)
    CIEDE2000::LAB targetLab;
    rgbToLAB(target_r, target_g, target_b, targetLab);

    uint32_t colorsChecked = 0;
    unsigned long const START_TIME = millis();
    const unsigned long MAX_SEARCH_TIME = 2000;  // Max 2000ms search time for thorough search

    while (readNextColor(currentColor)) {
      // Timeout check for responsive live view
      if (millis() - START_TIME > MAX_SEARCH_TIME) {
        Serial.printf("Search timeout after %ums, checked %u colors\n", MAX_SEARCH_TIME,
                      colorsChecked);
        break;
      }
      // Calculate distance - use simple RGB for whites/near-whites, CIEDE2000 for others
      float distance = NAN;

      // Check if both colors are light (potential whites)
      bool const IS_LIGHT_TARGET = (target_r > 200 && target_g > 200 && target_b > 200);
      bool const IS_LIGHT_CURRENT =
          (currentColor.r > 200 && currentColor.g > 200 && currentColor.b > 200);

      if (IS_LIGHT_TARGET && IS_LIGHT_CURRENT) {
        // Use simple RGB distance for whites/light colors (more accurate for close matches)
        auto const DR = (float)(target_r - currentColor.r);
        auto const DG = (float)(target_g - currentColor.g);
        auto const DB = (float)(target_b - currentColor.b);
        distance = sqrt((DR * DR) + (DG * DG) + (DB * DB));
      } else {
        // Use CIEDE2000 for other colors
        CIEDE2000::LAB currentLab;
        rgbToLAB(currentColor.r, currentColor.g, currentColor.b, currentLab);
        distance = (float)CIEDE2000::ciedE2000(targetLab, currentLab);
      }

      if (distance < minDistance) {
        minDistance = distance;
        result = currentColor;
        found = true;

        // Debug output for very close matches
        if ((IS_LIGHT_TARGET && IS_LIGHT_CURRENT && distance < 10.0f) ||
            (!IS_LIGHT_TARGET || !IS_LIGHT_CURRENT) && distance < 5.0f) {
          Serial.printf("Close match: %s (%d,%d,%d) distance: %.2f\n", currentColor.name,
                        currentColor.r, currentColor.g, currentColor.b, distance);
        }

        // Early exit for very close matches
        if ((IS_LIGHT_TARGET && IS_LIGHT_CURRENT && distance < 3.0f) ||
            ((!IS_LIGHT_TARGET || !IS_LIGHT_CURRENT) && distance < 1.0f)) {
          Serial.printf("Excellent match found, stopping search\n");
          break;
        }
      }

      colorsChecked++;

      // Progress indicator (less frequent)
      if (colorsChecked % 2000 == 0) {
        Serial.printf("Searching... checked %u colors\n", colorsChecked);
      }
    }

    if (found) {
      // Cache the result
      cache.r = target_r;
      cache.g = target_g;
      cache.b = target_b;
      cache.result = result;
      cache.valid = true;

      Serial.printf("Final match: %s (%d,%d,%d) distance: %.2f after checking %u colors\n",
                    result.name, result.r, result.g, result.b, minDistance, colorsChecked);
    }

    return found;
  }

  /**
   * @brief Close the database file
   */
  void close() {
    if (file_open) {
      file.close();
      file_open = false;
    }
  }
};

#endif  // DULUX_SIMPLE_READER_H
