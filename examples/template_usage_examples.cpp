/*!
 * @file template_usage_examples.cpp
 * @brief Examples demonstrating C++ Core Guidelines template usage
 * @brief Shows how to use the new template-based PSRAM utilities
 * 
 * This file demonstrates the proper usage of template-based containers
 * and memory management utilities following C++ Core Guidelines.
 * 
 * @author Color Calibration System
 * @version 1.0
 * @date 2025-07-23
 */

#include "psram_utils.h"
#include "Arduino.h"

// Example structures for demonstration
struct ColorData {
  uint8_t r, g, b;
  String name;
  float brightness;
  
  ColorData() : r(0), g(0), b(0), brightness(0.0f) {}
  ColorData(uint8_t red, uint8_t green, uint8_t blue, const String& n, float bright)
    : r(red), g(green), b(blue), name(n), brightness(bright) {}
};

// Example 1: Type-safe PSRAM vector usage
void example_psram_vector() {
  Serial.println("=== Example 1: Type-safe PSRAM Vector ===");
  
  // Create a type-safe PSRAM vector for color data
  PSRAMVector<ColorData> colors;
  
  // Check if we can allocate memory before proceeding
  if (!PSRAMUtils::can_allocate_in_psram<ColorData>(100)) {
    Serial.println("Insufficient PSRAM for 100 ColorData objects");
    return;
  }
  
  // Safe reserve with memory checking
  if (!PSRAMUtils::safe_reserve(colors, 100)) {
    Serial.println("Failed to reserve space for 100 colors");
    return;
  }
  
  // Add some sample colors
  colors.emplace_back(255, 0, 0, "Red", 0.3f);
  colors.emplace_back(0, 255, 0, "Green", 0.6f);
  colors.emplace_back(0, 0, 255, "Blue", 0.1f);
  
  Serial.printf("Created vector with %zu colors\n", colors.size());
  Serial.printf("Memory usage: %zu bytes\n", PSRAMUtils::get_memory_usage(colors));
  
  // Type-safe iteration
  for (const auto& color : colors) {
    Serial.printf("Color: %s RGB(%d,%d,%d) Brightness: %.2f\n", 
                  color.name.c_str(), color.r, color.g, color.b, color.brightness);
  }
}

// Example 2: RAII PSRAM array usage
void example_psram_array() {
  Serial.println("\n=== Example 2: RAII PSRAM Array ===");
  
  try {
    // Create a RAII-managed PSRAM array
    auto sensor_readings = PSRAMUtils::make_psram_array<float>(1000);
    
    // Fill with sample data
    for (size_t i = 0; i < sensor_readings.size(); ++i) {
      sensor_readings[i] = sin(i * 0.1f) * 100.0f;
    }
    
    Serial.printf("Created array with %zu elements\n", sensor_readings.size());
    
    // Calculate statistics
    float sum = 0.0f;
    float min_val = sensor_readings[0];
    float max_val = sensor_readings[0];
    
    for (const auto& value : sensor_readings) {
      sum += value;
      if (value < min_val) min_val = value;
      if (value > max_val) max_val = value;
    }
    
    float average = sum / sensor_readings.size();
    Serial.printf("Statistics: Min=%.2f, Max=%.2f, Avg=%.2f\n", min_val, max_val, average);
    
    // Array automatically deallocated when going out of scope
  } catch (const std::bad_alloc& e) {
    Serial.println("Failed to allocate PSRAM array");
  }
}

// Example 3: Memory monitoring and safe operations
void example_memory_monitoring() {
  Serial.println("\n=== Example 3: Memory Monitoring ===");
  
  // Get comprehensive memory information
  auto mem_info = PSRAMUtils::get_memory_info();
  
  Serial.println("Memory Status:");
  Serial.printf("  Heap: %zu KB free / %zu KB total (%.1f%% used)\n",
                mem_info.free_heap / 1024, mem_info.total_heap / 1024, 
                mem_info.heap_usage_percent);
  Serial.printf("  PSRAM: %zu KB free / %zu KB total (%.1f%% used)\n",
                mem_info.free_psram / 1024, mem_info.total_psram / 1024, 
                mem_info.psram_usage_percent);
  
  // Check if we have sufficient memory for a large operation
  size_t large_allocation = 1024 * 1024; // 1MB
  if (PSRAMUtils::has_sufficient_memory(large_allocation)) {
    Serial.println("✓ Sufficient memory for 1MB allocation");
  } else {
    Serial.println("✗ Insufficient memory for 1MB allocation");
  }
  
  // Demonstrate safe container operations
  PSRAMVector<uint32_t> large_data;
  
  if (PSRAMUtils::safe_resize(large_data, 10000)) {
    Serial.println("✓ Successfully resized vector to 10,000 elements");
    
    // Fill with data
    for (size_t i = 0; i < large_data.size(); ++i) {
      large_data[i] = i * i;
    }
    
    Serial.printf("Memory usage: %zu KB\n", PSRAMUtils::get_memory_usage(large_data) / 1024);
  } else {
    Serial.println("✗ Failed to resize vector - insufficient memory");
  }
}

// Example 4: Template-based color processing
template<typename ColorContainer>
void process_colors(ColorContainer& colors) {
  Serial.println("\n=== Example 4: Template-based Color Processing ===");
  
  // This function works with any container of ColorData objects
  Serial.printf("Processing %zu colors...\n", colors.size());
  
  // Find brightest color
  auto brightest = std::max_element(colors.begin(), colors.end(),
    [](const ColorData& a, const ColorData& b) {
      return a.brightness < b.brightness;
    });
  
  if (brightest != colors.end()) {
    Serial.printf("Brightest color: %s (%.2f)\n", 
                  brightest->name.c_str(), brightest->brightness);
  }
  
  // Calculate average brightness
  float total_brightness = 0.0f;
  for (const auto& color : colors) {
    total_brightness += color.brightness;
  }
  
  float avg_brightness = total_brightness / colors.size();
  Serial.printf("Average brightness: %.2f\n", avg_brightness);
  
  // Count colors above average
  size_t above_average = std::count_if(colors.begin(), colors.end(),
    [avg_brightness](const ColorData& color) {
      return color.brightness > avg_brightness;
    });
  
  Serial.printf("Colors above average: %zu/%zu\n", above_average, colors.size());
}

// Example 5: Type-safe raw allocation
void example_raw_allocation() {
  Serial.println("\n=== Example 5: Type-safe Raw Allocation ===");
  
  // Allocate raw memory in a type-safe way
  constexpr size_t BUFFER_SIZE = 512;
  uint16_t* sensor_buffer = PSRAMUtils::allocate_psram<uint16_t>(BUFFER_SIZE);
  
  if (sensor_buffer) {
    Serial.printf("✓ Allocated %zu uint16_t elements in PSRAM\n", BUFFER_SIZE);
    
    // Fill buffer with sample sensor data
    for (size_t i = 0; i < BUFFER_SIZE; ++i) {
      sensor_buffer[i] = static_cast<uint16_t>(random(0, 65536));
    }
    
    // Process data
    uint32_t sum = 0;
    for (size_t i = 0; i < BUFFER_SIZE; ++i) {
      sum += sensor_buffer[i];
    }
    
    uint16_t average = static_cast<uint16_t>(sum / BUFFER_SIZE);
    Serial.printf("Average sensor value: %u\n", average);
    
    // Clean up - type-safe deallocation
    PSRAMUtils::deallocate_psram(sensor_buffer, BUFFER_SIZE);
    Serial.println("✓ Memory deallocated safely");
  } else {
    Serial.println("✗ Failed to allocate sensor buffer");
  }
}

// Main demonstration function
void demonstrate_template_usage() {
  Serial.println("C++ Core Guidelines Template Usage Examples");
  Serial.println("==========================================");
  
  example_psram_vector();
  example_psram_array();
  example_memory_monitoring();
  
  // Create sample data for template processing
  PSRAMVector<ColorData> sample_colors;
  sample_colors.emplace_back(255, 100, 100, "Light Red", 0.7f);
  sample_colors.emplace_back(100, 255, 100, "Light Green", 0.8f);
  sample_colors.emplace_back(100, 100, 255, "Light Blue", 0.4f);
  sample_colors.emplace_back(200, 200, 50, "Yellow", 0.9f);
  
  process_colors(sample_colors);
  example_raw_allocation();
  
  Serial.println("\n=== Template Usage Examples Complete ===");
}
