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

// Binary format constants
#define DULUX_MAGIC_NUMBER 0x584C5544  // "DULX" in little-endian
#define DULUX_BINARY_VERSION 1

/**
 * @brief Simple color structure for streaming access
 */
struct SimpleColor {
    uint8_t r, g, b;        // RGB values
    uint16_t lrv_scaled;    // LRV * 100
    uint32_t id;            // Color ID
    char name[64];          // Fixed-size name buffer
    char code[16];          // Fixed-size code buffer
    bool light_text;        // Light text flag
    
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
    File file;
    uint32_t total_colors;
    uint32_t current_position;
    bool file_open;

    // Simple cache for last result
    struct ColorCache {
        uint8_t r, g, b;
        SimpleColor result;
        bool valid;
    } cache;
    
    /**
     * @brief Read a string from file with length prefix into fixed buffer
     */
    bool readStringToBuffer(char* buffer, size_t buffer_size) {
        uint8_t length = file.read();
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
            size_t bytes_read = file.readBytes(buffer, length);
            if (bytes_read != length) {
                Serial.printf("String read error: expected %d, got %d\n", length, bytes_read);
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
    DuluxSimpleReader() : total_colors(0), current_position(0), file_open(false) {
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
        uint32_t magic, version, color_count, reserved;
        
        if (file.readBytes((char*)&magic, 4) != 4 ||
            file.readBytes((char*)&version, 4) != 4 ||
            file.readBytes((char*)&color_count, 4) != 4 ||
            file.readBytes((char*)&reserved, 4) != 4) {
            Serial.println("Failed to read header");
            file.close();
            return false;
        }
        
        if (magic != DULUX_MAGIC_NUMBER) {
            Serial.printf("Invalid magic: 0x%08X\n", magic);
            file.close();
            return false;
        }
        
        if (version != DULUX_BINARY_VERSION) {
            Serial.printf("Invalid version: %u\n", version);
            file.close();
            return false;
        }
        
        total_colors = color_count;
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
        int r = file.read();
        int g = file.read();
        int b = file.read();
        
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
        int light_flag = file.read();
        if (light_flag < 0) {
            Serial.printf("Failed to read light flag at position %u\n", current_position);
            return false;
        }
        color.light_text = (light_flag != 0);
        
        current_position++;
        return true;
    }
    
    /**
     * @brief Reset to beginning of color data
     */
    bool reset() {
        if (!file_open) return false;
        
        // Seek to start of color data (after 16-byte header)
        file.seek(16);
        current_position = 0;
        return true;
    }
    
    /**
     * @brief Find closest color by scanning through all colors
     */
    bool findClosestColor(uint8_t target_r, uint8_t target_g, uint8_t target_b, SimpleColor& result) {
        if (!file_open) return false;

        // Check cache first
        if (cache.valid && cache.r == target_r && cache.g == target_g && cache.b == target_b) {
            result = cache.result;
            Serial.printf("Cache hit for RGB(%d,%d,%d)\n", target_r, target_g, target_b);
            return true;
        }

        // Reset to beginning
        if (!reset()) return false;
        
        float min_distance = 999999.0f;
        bool found = false;
        SimpleColor current_color;
        
        Serial.printf("Searching for closest match to RGB(%d,%d,%d)\n", target_r, target_g, target_b);
        
        uint32_t colors_checked = 0;
        while (readNextColor(current_color)) {
            // Calculate distance
            float dr = (float)target_r - (float)current_color.r;
            float dg = (float)target_g - (float)current_color.g;
            float db = (float)target_b - (float)current_color.b;
            float distance = sqrt(dr*dr + dg*dg + db*db);
            
            if (distance < min_distance) {
                min_distance = distance;
                result = current_color;
                found = true;
                
                // Early exit for perfect matches
                if (distance < 0.1f) {
                    Serial.printf("Perfect match found after checking %u colors\n", colors_checked + 1);
                    break;
                }
            }
            
            colors_checked++;
            
            // Progress indicator
            if (colors_checked % 1000 == 0) {
                Serial.printf("Checked %u colors, best distance so far: %.2f\n", colors_checked, min_distance);
            }
        }
        
        if (found) {
            Serial.printf("Best match: %s (%s) - distance: %.2f\n",
                         result.name, result.code, min_distance);

            // Cache the result
            cache.r = target_r;
            cache.g = target_g;
            cache.b = target_b;
            cache.result = result;
            cache.valid = true;
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

#endif // DULUX_SIMPLE_READER_H
