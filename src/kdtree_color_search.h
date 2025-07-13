#ifndef KDTREE_COLOR_SEARCH_H
#define KDTREE_COLOR_SEARCH_H

#include <vector>
#include <algorithm>
#include <cmath>
#include <memory>

struct ColorPoint {
    uint8_t r, g, b;
    String name;
    String code;
    int index;
    
    ColorPoint() : r(0), g(0), b(0), index(-1) {}
    ColorPoint(uint8_t r, uint8_t g, uint8_t b, const String& name, const String& code, int idx)
        : r(r), g(g), b(b), name(name), code(code), index(idx) {}
};

struct KDNode {
    ColorPoint point;
    std::unique_ptr<KDNode> left;
    std::unique_ptr<KDNode> right;
    int axis; // 0=R, 1=G, 2=B
    
    KDNode(const ColorPoint& p, int a) : point(p), axis(a) {}
};

class KDTreeColorSearch {
private:
    std::unique_ptr<KDNode> root;
    ColorPoint bestMatch;
    float bestDistance;
    
    // Get coordinate value by axis (0=R, 1=G, 2=B)
    uint8_t getCoord(const ColorPoint& p, int axis) const {
        switch(axis) {
            case 0: return p.r;
            case 1: return p.g;
            case 2: return p.b;
            default: return 0;
        }
    }
    
    // Calculate squared Euclidean distance (faster than full distance)
    float distanceSquared(const ColorPoint& a, const ColorPoint& b) const {
        float dr = (float)a.r - (float)b.r;
        float dg = (float)a.g - (float)b.g;
        float db = (float)a.b - (float)b.b;
        return dr*dr + dg*dg + db*db;
    }
    
    // Comparison function for sorting
    struct AxisComparator {
        int axis;
        const KDTreeColorSearch* tree;
        
        AxisComparator(int a, const KDTreeColorSearch* t) : axis(a), tree(t) {}
        
        bool operator()(const ColorPoint& a, const ColorPoint& b) const {
            return tree->getCoord(a, axis) < tree->getCoord(b, axis);
        }
    };
    
    // Build k-d tree recursively with stack depth limit
    std::unique_ptr<KDNode> buildTree(std::vector<ColorPoint>& points, int depth) {
        if (points.empty()) return std::unique_ptr<KDNode>();
        
        // Prevent stack overflow for large datasets
        if (depth > 50) {
            // For very deep trees, create a leaf node with the first point
            return std::unique_ptr<KDNode>(new KDNode(points[0], depth % 3));
        }
        
        int axis = depth % 3; // Cycle through R, G, B
        
        // Sort points by current axis using comparator
        std::sort(points.begin(), points.end(), AxisComparator(axis, this));
        
        // Choose median as root
        size_t median = points.size() / 2;
        std::unique_ptr<KDNode> node(new KDNode(points[median], axis));
        
        // Recursively build left and right subtrees with size limits
        if (median > 0 && median <= 1000) { // Limit subtree size
            std::vector<ColorPoint> leftPoints(points.begin(), points.begin() + median);
            node->left = buildTree(leftPoints, depth + 1);
        }
        
        if (median + 1 < points.size() && (points.size() - median - 1) <= 1000) { // Limit subtree size
            std::vector<ColorPoint> rightPoints(points.begin() + median + 1, points.end());
            node->right = buildTree(rightPoints, depth + 1);
        }
        
        return node;
    }
    
    // Search k-d tree for nearest neighbor
    void searchNearest(const std::unique_ptr<KDNode>& node, const ColorPoint& target) {
        if (!node) return;
        
        // Check current node
        float dist = distanceSquared(node->point, target);
        if (dist < bestDistance) {
            bestDistance = dist;
            bestMatch = node->point;
        }
        
        // Determine which side to search first
        int axis = node->axis;
        uint8_t targetCoord = getCoord(target, axis);
        uint8_t nodeCoord = getCoord(node->point, axis);
        
        std::unique_ptr<KDNode>* nearSide = (targetCoord < nodeCoord) ? &node->left : &node->right;
        std::unique_ptr<KDNode>* farSide = (targetCoord < nodeCoord) ? &node->right : &node->left;
        
        // Search near side first
        searchNearest(*nearSide, target);
        
        // Check if we need to search far side
        float axisDistance = (float)targetCoord - (float)nodeCoord;
        if (axisDistance * axisDistance < bestDistance) {
            searchNearest(*farSide, target);
        }
    }
    
public:
    KDTreeColorSearch() : root(nullptr) {}
    
    // Build tree from color database
    void buildFromDatabase(const std::vector<ColorPoint>& colors) {
        if (colors.empty()) return;
        
        std::vector<ColorPoint> points = colors; // Copy for sorting
        root = buildTree(points, 0);
    }
    
    // Find closest color match
    ColorPoint findClosest(uint8_t r, uint8_t g, uint8_t b) {
        if (!root) return ColorPoint();
        
        ColorPoint target(r, g, b, "", "", -1);
        bestDistance = std::numeric_limits<float>::max();
        bestMatch = ColorPoint();
        
        searchNearest(root, target);
        return bestMatch;
    }
    
    // Get tree size (for debugging)
    size_t getSize() const {
        return countNodes(root);
    }
    
private:
    size_t countNodes(const std::unique_ptr<KDNode>& node) const {
        if (!node) return 0;
        return 1 + countNodes(node->left) + countNodes(node->right);
    }
};

#endif // KDTREE_COLOR_SEARCH_H
