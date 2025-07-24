/**
 * @file dulux_binary_reader.h
 * @brief Efficient binary reader for Dulux color database
 *
 * This header provides memory-efficient reading of the binary Dulux color database.
 * The binary format significantly reduces memory usage compared to JSON parsing.
 *
 * Binary Format:
 * - Header: 16 bytes (magic, version, count, reserved)
 * - Each color entry: variable length with packed data
 *
 * Memory Benefits:
 * - No JSON parsing overhead (saves ~1-2MB during loading)
 * - Compact binary format (typically 50-70% smaller than JSON)
 * - Direct memory mapping possible for read-only access
 * - Faster loading times
 */

#ifndef DULUX_BINARY_READER_H
#define DULUX_BINARY_READER_H

#include <Arduino.h>

#include <esp_heap_caps.h>

#include <LittleFS.h>

#include <new>

// Binary format constants
#define DULUX_MAGIC_NUMBER 0x584C5544  // "DULX" in little-endian
#define DULUX_BINARY_VERSION 1
#define DULUX_HEADER_SIZE 16

/**
 * @brief Compact color structure for binary format
 *
 * This structure is optimized for memory usage while maintaining
 * all necessary color information.
 */
struct DuluxColorBinary {
  uint8_t r, g, b;      // RGB values (3 bytes)
  uint16_t lrv_scaled;  // LRV * 100 (2 bytes, e.g., 7940 for 79.40)
  uint32_t id;          // Color ID (4 bytes)
  String name;          // Color name (heap allocated)
  String code;          // Color code (heap allocated)
  bool light_text;      // Light text flag (1 byte)

  /**
   * @brief Get LRV as float value
   * @return LRV value (e.g., 79.40)
   */
  float getLRV() const {
    return lrv_scaled / 100.0f;
  }

  /**
   * @brief Get LRV as string for compatibility
   * @return LRV as string (e.g., "79.40")
   */
  String getLRVString() const {
    float lrv = getLRV();
    return String(lrv, 2);  // 2 decimal places
  }
};

/**
 * @brief Binary header structure
 */
struct DuluxBinaryHeader {
  uint32_t magic;        // Magic number for validation
  uint32_t version;      // Format version
  uint32_t color_count;  // Number of colors in file
  uint32_t reserved;     // Reserved for future use
};

/**
 * @brief Dulux binary database reader class
 *
 * This class provides efficient loading and access to the binary
 * Dulux color database with minimal memory overhead.
 */
class DuluxBinaryReader {
 private:
  DuluxColorBinary* colors;
  uint32_t color_count;
  bool loaded;

  /**
   * @brief Read a string from file with length prefix
   * @param file File to read from
   * @return String read from file
   */
  String readString(File& file) {
    uint8_t length = file.read();
    if (length == 0 || length == 255) {  // 255 indicates read error
      return String();
    }

    // Use heap allocation instead of PSRAM for temporary buffer
    char* buffer = static_cast<char*>(malloc(length + 1));
    if (!buffer) {
      Serial.printf("Failed to allocate %d bytes for string buffer\n", length + 1);
      return String();
    }

    size_t bytes_read = file.readBytes(buffer, length);
    if (bytes_read != length) {
      Serial.printf("String read error: expected %d bytes, got %d\n", length, bytes_read);
      free(buffer);
      return String();
    }

    buffer[bytes_read] = '\0';

    // Create string and immediately free buffer
    String result(buffer);
    free(buffer);

    return result;
  }

 public:
  /**
   * @brief Constructor
   */
  DuluxBinaryReader() : colors(nullptr), color_count(0), loaded(false) {
  }

  /**
   * @brief Destructor - cleanup allocated memory
   */
  ~DuluxBinaryReader() {
    if (colors) {
      delete[] colors;
      colors = nullptr;
    }
  }

  /**
   * @brief Load binary color database from file
   * @param filename Path to binary file (e.g., "/dulux.bin")
   * @return true if loaded successfully, false otherwise
   */
  bool loadDatabase(const char* filename) {
    Serial.printf("Loading binary color database: %s\n", filename);

    // Open file
    File file = LittleFS.open(filename, "r");
    if (!file) {
      Serial.printf("Failed to open binary file: %s\n", filename);
      return false;
    }

    // Read and validate header
    DuluxBinaryHeader header;
    if (file.readBytes(reinterpret_cast<char*>(&header), sizeof(header)) != sizeof(header)) {
      Serial.println("Failed to read binary header");
      file.close();
      return false;
    }

    // Validate magic number and version
    if (header.magic != DULUX_MAGIC_NUMBER) {
      Serial.printf("Invalid magic number: 0x%08X (expected 0x%08X)\n", header.magic,
                    DULUX_MAGIC_NUMBER);
      file.close();
      return false;
    }

    if (header.version != DULUX_BINARY_VERSION) {
      Serial.printf("Unsupported version: %u (expected %u)\n", header.version,
                    DULUX_BINARY_VERSION);
      file.close();
      return false;
    }

    color_count = header.color_count;
    Serial.printf("Binary database contains %u colors\n", color_count);

// Debug mode: limit colors for testing
#ifdef DEBUG_BINARY_LOADING
    if (color_count > 100) {
      Serial.printf("DEBUG: Limiting to 100 colors for testing\n");
      color_count = 100;
    }
#endif

    // Allocate memory in regular heap (PSRAM can cause issues with String objects)
    Serial.printf("Allocating %u bytes for %u colors...\n", sizeof(DuluxColorBinary) * color_count,
                  color_count);

    colors = new (std::nothrow) DuluxColorBinary[color_count];
    if (!colors) {
      Serial.println("Failed to allocate memory for color database");
      file.close();
      return false;
    }

    Serial.println("Memory allocation successful");

    Serial.printf("Allocated %u bytes for color database\n",
                  sizeof(DuluxColorBinary) * color_count);

    // Read color entries with error checking
    Serial.println("Starting to read color entries...");

    for (uint32_t i = 0; i < color_count; i++) {
      DuluxColorBinary& color = colors[i];

      // Check if we can still read from file
      if (!file.available()) {
        Serial.printf("Unexpected end of file at color %u\n", i);
        delete[] colors;
        colors = nullptr;
        file.close();
        return false;
      }

      // Read RGB values with validation
      int r_val = file.read();
      int g_val = file.read();
      int b_val = file.read();

      if (r_val < 0 || g_val < 0 || b_val < 0) {
        Serial.printf("Failed to read RGB values for color %u\n", i);
        delete[] colors;
        colors = nullptr;
        file.close();
        return false;
      }

      color.r = (uint8_t)r_val;
      color.g = (uint8_t)g_val;
      color.b = (uint8_t)b_val;

      // Read LRV (scaled) with validation
      if (file.readBytes(reinterpret_cast<char*>(&color.lrv_scaled), sizeof(color.lrv_scaled)) !=
          sizeof(color.lrv_scaled)) {
        Serial.printf("Failed to read LRV for color %u\n", i);
        delete[] colors;
        colors = nullptr;
        file.close();
        return false;
      }

      // Read ID with validation
      if (file.readBytes(reinterpret_cast<char*>(&color.id), sizeof(color.id)) != sizeof(color.id)) {
        Serial.printf("Failed to read ID for color %u\n", i);
        delete[] colors;
        colors = nullptr;
        file.close();
        return false;
      }

      // Read name and code strings with error checking
      color.name = readString(file);
      if (color.name.length() == 0) {
        Serial.printf("Warning: Empty name for color %u\n", i);
      }

      color.code = readString(file);
      if (color.code.length() == 0) {
        Serial.printf("Warning: Empty code for color %u\n", i);
      }

      // Read light text flag
      int light_flag = file.read();
      if (light_flag < 0) {
        Serial.printf("Failed to read light text flag for color %u\n", i);
        delete[] colors;
        colors = nullptr;
        file.close();
        return false;
      }
      color.light_text = (light_flag != 0);

      // Progress indicator for large databases
      if ((i + 1) % 500 == 0) {
        Serial.printf("Loaded %u/%u colors...\n", i + 1, color_count);
      }
    }

    file.close();
    loaded = true;

    Serial.printf("Successfully loaded %u colors from binary database\n", color_count);
    Serial.printf("Memory usage: %u bytes\n", sizeof(DuluxColorBinary) * color_count);

    return true;
  }

  /**
   * @brief Check if database is loaded
   * @return true if loaded, false otherwise
   */
  bool isLoaded() const {
    return loaded && colors != nullptr;
  }

  /**
   * @brief Get number of colors in database
   * @return Number of colors
   */
  uint32_t getColorCount() const {
    return color_count;
  }

  /**
   * @brief Get color by index
   * @param index Color index (0 to getColorCount()-1)
   * @return Pointer to color, or nullptr if invalid index
   */
  const DuluxColorBinary* getColor(uint32_t index) const {
    if (!loaded || index >= color_count) {
      return nullptr;
    }
    return &colors[index];
  }

  /**
   * @brief Find closest color match using RGB values
   * @param r Red component (0-255)
   * @param g Green component (0-255)
   * @param b Blue component (0-255)
   * @return Pointer to closest color, or nullptr if database not loaded
   */
  const DuluxColorBinary* findClosestColor(uint8_t r, uint8_t g, uint8_t b) const {
    if (!loaded || color_count == 0) {
      return nullptr;
    }

    const DuluxColorBinary* closest = nullptr;
    float min_distance = 999999.0f;

    for (uint32_t i = 0; i < color_count; i++) {
      const DuluxColorBinary& color = colors[i];

      // Calculate Euclidean distance in RGB space
      float dr = (float)r - (float)color.r;
      float dg = (float)g - (float)color.g;
      float db = (float)b - (float)color.b;
      float distance = sqrt(dr * dr + dg * dg + db * db);

      if (distance < min_distance) {
        min_distance = distance;
        closest = &color;

        // Early exit for perfect matches
        if (distance < 0.1f) {
          break;
        }
      }
    }

    return closest;
  }
};

#endif  // DULUX_BINARY_READER_H
