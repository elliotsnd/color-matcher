/*!
 * @file lightweight_kdtree.h
 * @brief Lightweight, iterative K-D tree for embedded systems
 * Optimized for ESP32 with large color datasets (4500+ colors)
 * Uses iterative construction to avoid stack overflow
 * Memory-optimized with contiguous allocation
 */

#ifndef LIGHTWEIGHT_KDTREE_H
#define LIGHTWEIGHT_KDTREE_H

#include <Arduino.h>

#include <esp_heap_caps.h>

#include <algorithm>
#include <queue>
#include <vector>

// Custom PSRAM allocator for STL containers
template <typename T>
class PSRAMAllocator {
 public:
  using value_type = T;
  using pointer = T*;
  using const_pointer = T*;
  using reference = T&;
  using const_reference = T&;
  using size_type = size_t;
  using difference_type = ptrdiff_t;

  template <typename U>
  struct Rebind {
    using other = PSRAMAllocator<U>;
  };

  PSRAMAllocator() = default;
  template <typename U>
  explicit PSRAMAllocator(const PSRAMAllocator<U>& /*unused*/) {
  }

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

  void deallocate(pointer p, size_type /*unused*/) {
    heap_caps_free(p);
  }

  bool operator==(const PSRAMAllocator& /*unused*/) const {
    return true;
  }
  bool operator!=(const PSRAMAllocator& /*unused*/) const {
    return false;
  }
};

// Type aliases for PSRAM-allocated vectors
using PSRAMColorVector = std::vector<struct ColorPoint, PSRAMAllocator<struct ColorPoint>>;
using PSRAMNodeVector = std::vector<struct KDNode, PSRAMAllocator<struct KDNode>>;

// Compact color point structure
struct ColorPoint {
  uint8_t r, g, b;  // RGB values (3 bytes)
  uint16_t index;   // Index in original database (2 bytes)

  ColorPoint() : r(0), g(0), b(0), index(0) {
  }
  ColorPoint(uint8_t r, uint8_t g, uint8_t b, uint16_t idx) : r(r), g(g), b(b), index(idx) {
  }
};

// Compact tree node structure
struct KDNode {
  ColorPoint point;  // Color data (5 bytes)
  uint8_t axis{0};   // Splitting axis 0=R, 1=G, 2=B (1 byte)
  uint16_t left{0};  // Index of left child (2 bytes, 0 = no child)
  uint16_t right{0};  // Index of right child (2 bytes, 0 = no child)

  KDNode() = default;
};

class LightweightKDTree {
 private:
  PSRAMNodeVector nodes;
  PSRAMColorVector points;
  size_t node_count{0};
  bool built{false};
  size_t max_tree_size{0};  // Adaptive size limit based on available memory

  // Build queue item for iterative construction
  struct BuildItem {
    uint16_t node_index;
    size_t start;
    size_t end;
    uint8_t depth;

    BuildItem(uint16_t ni, size_t s, size_t e, uint8_t d)
        : node_index(ni), start(s), end(e), depth(d) {
    }
  };

  // Get coordinate value for given axis
  static uint8_t getCoordinate(const ColorPoint& p, uint8_t axis) {
    switch (axis) {
      case 0:
        return p.r;
      case 1:
        return p.g;
      case 2:
        return p.b;
      default:
        return p.r;
    }
  }

  // Calculate squared Euclidean distance
  static uint32_t distanceSquared(const ColorPoint& a, const ColorPoint& b) {
    int32_t const DR = (int32_t)a.r - (int32_t)b.r;
    int32_t const DG = (int32_t)a.g - (int32_t)b.g;
    int32_t const DB = (int32_t)a.b - (int32_t)b.b;
    return (uint32_t)((DR * DR) + (DG * DG) + (DB * DB));
  }

  // Iterative nearest neighbor search
  static void searchNearest(uint16_t node_index, const ColorPoint& target, ColorPoint& best,
                            uint32_t& best_dist) {
    if (node_index == 0) {
      return;
    }

    // Use a stack for iterative traversal to avoid recursion
    std::vector<uint16_t> stack;
    stack.reserve(64);  // Reasonable depth limit
    stack.push_back(node_index);

    while (!stack.empty() && stack.size() < 64) {  // Safety limit
      uint16_t current = stack.back();
      stack.pop_back();

      const KDNode& node = nodes[current - 1];  // Convert to 0-based index

      // Check current node
      uint32_t const DIST = distanceSquared(node.point, target);
      if (DIST < best_dist) {
        best_dist = DIST;
        best = node.point;
      }

      // Determine which side to search first
      uint8_t const TARGET_COORD = getCoordinate(target, node.axis);
      uint8_t const NODE_COORD = getCoordinate(node.point, node.axis);

      uint16_t firstChild = 0;
      uint16_t secondChild = 0;
      if (TARGET_COORD <= NODE_COORD) {
        firstChild = node.left;
        secondChild = node.right;
      } else {
        firstChild = node.right;
        secondChild = node.left;
      }

      // Add children to stack (second child first so first child is processed first)
      if (secondChild != 0) {
        // Check if we need to search the other side
        int32_t const AXIS_DIST = (int32_t)TARGET_COORD - (int32_t)NODE_COORD;
        if ((uint32_t)(AXIS_DIST * AXIS_DIST) < best_dist) {
          stack.push_back(secondChild);
        }
      }

      if (firstChild != 0) {
        stack.push_back(firstChild);
      }
    }
  }

  // Build a balanced tree using iterative approach
  bool buildBalancedTree() {
    if (points.empty()) {
      return false;
    }

    // Use iterative breadth-first construction
    struct BuildTask {
      size_t node_idx;
      size_t start;
      size_t end;
      uint8_t depth;
    };

    std::queue<BuildTask> buildQueue;

    // Start with root
    build_queue.push({0, 0, points.size(), 0});
    node_count = 0;

    while (!build_queue.empty() && node_count < points.size()) {
      BuildTask task = build_queue.front();
      buildQueue.pop();

      if (task.start >= task.end) {
        continue;
      }

      // Choose splitting axis (cycle through R, G, B)
      uint8_t const AXIS = task.depth % 3;

      // Sort by current axis
      std::sort(points.begin() + task.start, points.begin() + task.end,
                [axis](const ColorPoint& a, const ColorPoint& b) {
                  switch (axis) {
                    case 0:
                      return a.r < b.r;
                    case 1:
                      return a.g < b.g;
                    case 2:
                      return a.b < b.b;
                    default:
                      return a.r < b.r;
                  }
                });

      // Find median
      size_t median = task.start + ((task.end - task.start) / 2);

      // Create node
      if (task.node_idx < nodes.size()) {
        KDNode& node = nodes[task.node_idx];
        node.point = points[median];
        node.axis = AXIS;
        node.left = 0;
        node.right = 0;

        node_count++;

        // Add children to queue if there's room and points
        if (median > task.start && node_count < nodes.size()) {
          size_t leftIdx = node_count;
          node.left = leftIdx + 1;  // 1-based indexing
          buildQueue.push({leftIdx, task.start, median, (uint8_t)(task.depth + 1)});
        }

        if (median + 1 < task.end && node_count < nodes.size()) {
          size_t rightIdx = node_count;
          node.right = rightIdx + 1;  // 1-based indexing
          buildQueue.push({rightIdx, median + 1, task.end, (uint8_t)(task.depth + 1)});
        }
      }

      // Progress reporting every 500 nodes
      if (node_count % 500 == 0) {
        Serial.printf("[KDTree] Built %u/%u nodes (PSRAM: %u KB free)\n", node_count, points.size(),
                      ESP.getFreePsram() / 1024);
        yield();  // Allow other tasks to run
      }

      // Safety check - if we're running low on memory, stop
      if (ESP.getFreePsram() < 500 * 1024) {  // Keep 500KB PSRAM free
        Serial.printf("[KDTree] Stopping at %u nodes to preserve PSRAM\n", node_count);
        break;
      }
    }

    return node_count > 0;
  }

 public:
  LightweightKDTree() : nodes(PSRAMAllocator<KDNode>()), points(PSRAMAllocator<ColorPoint>()) {
    // Calculate adaptive tree size based on available PSRAM
    calculateOptimalTreeSize();
  }

  ~LightweightKDTree() {
    clear();
  }

  // Calculate optimal tree size based on available PSRAM
  void calculateOptimalTreeSize() {
    size_t const FREE_PSRAM = ESP.getFreePsram() = 0;
    size_t const FREE_HEAP = ESP.getFreeHeap() = 0;

    // Reserve 2MB for other operations, use remaining for KD-tree
    size_t const AVAILABLE_MEMORY =
        FREE_PSRAM > 2 * 1024 * 1024 ? FREE_PSRAM - (2 * 1024 * 1024) : 0;

    // Each tree node uses sizeof(KDNode) + sizeof(ColorPoint) bytes
    size_t const BYTES_PER_POINT = sizeof(KDNode) + sizeof(ColorPoint);
    max_tree_size = AVAILABLE_MEMORY / BYTES_PER_POINT;

    // Cap at reasonable maximum (4500 colors total in database)
    max_tree_size = std::min<size_t>(max_tree_size, 4500);

    // Ensure minimum viable tree size
    max_tree_size = std::max<size_t>(max_tree_size, 500);

    Serial.printf("[KDTree] Available PSRAM: %u KB, Optimal tree size: %u colors\n",
                  FREE_PSRAM / 1024, max_tree_size);
  }

  void clear() {
    nodes.clear();
    points.clear();
    node_count = 0;
    built = false;
  }

  // Build tree from color points using iterative method with adaptive sizing
  bool build(const PSRAMColorVector& input_points) {
    Serial.println("[KDTree] Starting PSRAM-optimized iterative tree construction...");
    unsigned long const START_TIME = millis();

    if (input_points.empty()) {
      Serial.println("[KDTree] Error: No input points provided");
      return false;
    }

    clear();

    // Use adaptive sizing based on available PSRAM
    calculateOptimalTreeSize();
    size_t actualPoints = std::min(input_points.size(), max_tree_size);

    Serial.printf("[KDTree] Building tree with %u of %u colors (limit: %u)\n", actualPoints,
                  input_points.size(), max_tree_size);

    // Check PSRAM allocation specifically
    size_t const REQUIRED_MEMORY = actualPoints * (sizeof(KDNode) + sizeof(ColorPoint));
    size_t const FREE_PSRAM = ESP.getFreePsram() = 0;

    Serial.printf("[KDTree] Required: %u KB, Available PSRAM: %u KB\n", REQUIRED_MEMORY / 1024,
                  FREE_PSRAM / 1024);

    if (REQUIRED_MEMORY > FREE_PSRAM * 0.8) {  // Use only 80% of available PSRAM
      actualPoints = (FREE_PSRAM * 0.8) / (sizeof(KDNode) + sizeof(ColorPoint));
      Serial.printf("[KDTree] Reducing size to %u colors to fit in PSRAM\n", actualPoints);
    }

    if (actualPoints < 100) {
      Serial.println("[KDTree] Error: Insufficient PSRAM for meaningful tree");
      return false;
    }

    try {
      // Allocate in PSRAM using custom allocator
      points.clear();
      points.reserve(actual_points);

      // Copy subset of points
      for (size_t i = 0; i < actualPoints; i++) {
        points.push_back(input_points[i]);
      }

      nodes.clear();
      nodes.resize(actual_points);

      Serial.printf("[KDTree] PSRAM allocation successful for %u points\n", actualPoints);

      // Build a proper balanced tree iteratively
      if (buildBalancedTree()) {
        built = true;
        unsigned long const BUILD_TIME = millis() - START_TIME;

        Serial.printf("[KDTree] Tree built successfully in %u ms\n", BUILD_TIME);
        Serial.printf("[KDTree] Nodes: %u, Memory: %u KB PSRAM\n", node_count,
                      getMemoryUsage() / 1024);
        Serial.printf("[KDTree] Remaining PSRAM: %u KB\n", ESP.getFreePsram() / 1024);

        return true;
      }
    } catch (const std::exception& e) {
      Serial.printf("[KDTree] Error during PSRAM allocation: %s\n", e.what());
    }

    Serial.println("[KDTree] Failed to build tree");
    clear();
    return false;
  }

  // Find nearest neighbor using proper KD-tree search
  ColorPoint findNearest(uint8_t r, uint8_t g, uint8_t b) const {
    if (!built || node_count == 0) {
      Serial.println("[KDTree] Warning: Tree not built or empty");
      return {};
    }

    ColorPoint const TARGET(r, g, b, 0);
    ColorPoint best = nodes[0].point;  // Start with root
    uint32_t bestDist = distanceSquared(TARGET, best);

    // Use proper KD-tree search if we have a complete tree
    if (node_count > 1) {
      searchNearest(1, TARGET, best, bestDist);  // 1-based indexing
    }

    return best;
  }

  // Get tree statistics
  size_t getNodeCount() const {
    return node_count;
  }
  bool isBuilt() const {
    return built;
  }
  size_t getMemoryUsage() const {
    return node_count * sizeof(KDNode) + points.size() * sizeof(ColorPoint);
  }
};

#endif  // LIGHTWEIGHT_KDTREE_H
