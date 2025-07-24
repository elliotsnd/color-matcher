/*!
 * @file psram_utils.h
 * @brief Template-based PSRAM memory management utilities
 * @brief Following C++ Core Guidelines for type-safe container and memory management
 * 
 * This header provides template-based utilities for type-safe PSRAM allocation,
 * RAII wrappers, and container helpers optimized for ESP32 embedded systems.
 * 
 * @author Color Calibration System
 * @version 1.0
 * @date 2025-07-23
 */

#ifndef PSRAM_UTILS_H
#define PSRAM_UTILS_H

#include <Arduino.h>
#include <esp_heap_caps.h>
#include <vector>
#include <deque>
#include <memory>
#include <stdexcept>

// Template-based type-safe PSRAM allocator following C++ Core Guidelines
template<typename T>
class TypeSafePsramAllocator {
 public:
  using value_type = T;
  using pointer = T*;
  using const_pointer = const T*;
  using reference = T&;
  using const_reference = const T&;
  using size_type = size_t;
  using difference_type = ptrdiff_t;

  TypeSafePsramAllocator() = default;
  template<typename U>
  explicit TypeSafePsramAllocator(const TypeSafePsramAllocator<U>&) noexcept {}

  pointer allocate(size_type n) {
    void* ptr = heap_caps_malloc(n * sizeof(T), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!ptr) {
      // Fallback to regular heap if PSRAM allocation fails
      ptr = heap_caps_malloc(n * sizeof(T), MALLOC_CAP_DEFAULT);
    }
    if (!ptr) {
      throw std::bad_alloc();
    }
    return static_cast<pointer>(ptr);
  }

  void deallocate(pointer p, size_type) noexcept {
    heap_caps_free(p);
  }

  template<typename U>
  bool operator==(const TypeSafePsramAllocator<U>&) const noexcept { return true; }
  
  template<typename U>
  bool operator!=(const TypeSafePsramAllocator<U>&) const noexcept { return false; }
};

// Type aliases for commonly used PSRAM-allocated containers
template<typename T>
using PSRAMVector = std::vector<T, TypeSafePsramAllocator<T>>;

template<typename T>
using PSRAMDeque = std::deque<T, TypeSafePsramAllocator<T>>;

// Template-based memory management utilities
namespace PSRAMUtils {
  
  // Template function for type-safe PSRAM allocation
  template<typename T>
  T* allocate_psram(size_t count = 1) {
    TypeSafePsramAllocator<T> allocator;
    try {
      return allocator.allocate(count);
    } catch (const std::bad_alloc&) {
      return nullptr;
    }
  }
  
  // Template function for type-safe PSRAM deallocation
  template<typename T>
  void deallocate_psram(T* ptr, size_t count = 1) {
    if (ptr) {
      TypeSafePsramAllocator<T> allocator;
      allocator.deallocate(ptr, count);
    }
  }
  
  // RAII wrapper for PSRAM-allocated arrays
  template<typename T>
  class PSRAMArray {
  private:
    T* data_;
    size_t size_;
    TypeSafePsramAllocator<T> allocator_;
    
  public:
    explicit PSRAMArray(size_t size) : data_(nullptr), size_(size) {
      if (size > 0) {
        data_ = allocator_.allocate(size);
        if (!data_) {
          throw std::bad_alloc();
        }
        // Initialize elements
        for (size_t i = 0; i < size; ++i) {
          new(data_ + i) T();
        }
      }
    }
    
    ~PSRAMArray() {
      if (data_) {
        // Destroy elements
        for (size_t i = 0; i < size_; ++i) {
          data_[i].~T();
        }
        allocator_.deallocate(data_, size_);
      }
    }
    
    // Non-copyable but movable
    PSRAMArray(const PSRAMArray&) = delete;
    PSRAMArray& operator=(const PSRAMArray&) = delete;
    
    PSRAMArray(PSRAMArray&& other) noexcept 
      : data_(other.data_), size_(other.size_), allocator_(std::move(other.allocator_)) {
      other.data_ = nullptr;
      other.size_ = 0;
    }
    
    PSRAMArray& operator=(PSRAMArray&& other) noexcept {
      if (this != &other) {
        this->~PSRAMArray();
        data_ = other.data_;
        size_ = other.size_;
        allocator_ = std::move(other.allocator_);
        other.data_ = nullptr;
        other.size_ = 0;
      }
      return *this;
    }
    
    // Access operators
    T& operator[](size_t index) { return data_[index]; }
    const T& operator[](size_t index) const { return data_[index]; }
    
    // Utility methods
    T* data() { return data_; }
    const T* data() const { return data_; }
    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    
    // Iterator support
    T* begin() { return data_; }
    T* end() { return data_ + size_; }
    const T* begin() const { return data_; }
    const T* end() const { return data_ + size_; }
  };
  
  // Factory function for creating PSRAM arrays
  template<typename T>
  PSRAMArray<T> make_psram_array(size_t size) {
    return PSRAMArray<T>(size);
  }
  
  // Template function to get memory usage of containers
  template<typename Container>
  size_t get_memory_usage(const Container& container) {
    return container.size() * sizeof(typename Container::value_type);
  }
  
  // Template function to check if container fits in available PSRAM
  template<typename T>
  bool can_allocate_in_psram(size_t count) {
    size_t required_bytes = count * sizeof(T);
    size_t available_psram = ESP.getFreePsram();
    return available_psram > required_bytes;
  }
  
  // Template function for safe container resizing with memory checks
  template<typename Container>
  bool safe_resize(Container& container, size_t new_size) {
    using ValueType = typename Container::value_type;
    if (!can_allocate_in_psram<ValueType>(new_size)) {
      return false;
    }
    try {
      container.resize(new_size);
      return true;
    } catch (const std::bad_alloc&) {
      return false;
    }
  }
  
  // Template function for safe container reserve with memory checks
  template<typename Container>
  bool safe_reserve(Container& container, size_t capacity) {
    using ValueType = typename Container::value_type;
    if (!can_allocate_in_psram<ValueType>(capacity)) {
      return false;
    }
    try {
      container.reserve(capacity);
      return true;
    } catch (const std::bad_alloc&) {
      return false;
    }
  }
  
  // Memory monitoring utilities
  struct MemoryInfo {
    size_t free_heap;
    size_t free_psram;
    size_t total_heap;
    size_t total_psram;
    float heap_usage_percent;
    float psram_usage_percent;
  };
  
  // Get comprehensive memory information
  inline MemoryInfo get_memory_info() {
    MemoryInfo info;
    info.free_heap = ESP.getFreeHeap();
    info.free_psram = ESP.getFreePsram();
    info.total_heap = ESP.getHeapSize();
    info.total_psram = ESP.getPsramSize();
    
    info.heap_usage_percent = info.total_heap > 0 ? 
      (100.0f * (info.total_heap - info.free_heap)) / info.total_heap : 0.0f;
    info.psram_usage_percent = info.total_psram > 0 ? 
      (100.0f * (info.total_psram - info.free_psram)) / info.total_psram : 0.0f;
    
    return info;
  }
  
  // Check if system has sufficient memory for operation
  inline bool has_sufficient_memory(size_t required_bytes, float safety_margin = 0.1f) {
    MemoryInfo info = get_memory_info();
    size_t safe_threshold = static_cast<size_t>(required_bytes * (1.0f + safety_margin));
    return (info.free_psram > safe_threshold) || (info.free_heap > safe_threshold);
  }
}

#endif // PSRAM_UTILS_H
