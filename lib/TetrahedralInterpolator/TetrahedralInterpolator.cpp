/**
 * @file TetrahedralInterpolator.cpp
 * @brief Implementation of robust tetrahedral interpolation for 4-point color calibration
 */

#include "TetrahedralInterpolator.h"
#include "CIEDE2000.h"

// Constructor
TetrahedralInterpolator::TetrahedralInterpolator() {
    isInitialized = false;
    isValidTetrahedron = false;
    interpolationCount = 0;
    fallbackCount = 0;
}

// Initialize interpolator with calibration data
bool TetrahedralInterpolator::initialize(const ColorScience::CalibrationData& calibData) {
    Serial.println("=== Initializing Tetrahedral Interpolator ===");
    
    // Check if all 4 reference points are available
    if (!calibData.status.is4PointCalibrated()) {
        Serial.println("ERROR: 4-point calibration not complete");
        return false;
    }
    
    // Normalize reference points to [0,1] range based on sensor capabilities
    const float maxSensorValue = 65535.0f; // 16-bit sensor
    
    blackPoint = Point3D(
        calibData.blackReference.raw.X / maxSensorValue,
        calibData.blackReference.raw.Y / maxSensorValue,
        calibData.blackReference.raw.Z / maxSensorValue
    );
    
    whitePoint = Point3D(
        calibData.whiteReference.raw.X / maxSensorValue,
        calibData.whiteReference.raw.Y / maxSensorValue,
        calibData.whiteReference.raw.Z / maxSensorValue
    );
    
    bluePoint = Point3D(
        calibData.blueReference.raw.X / maxSensorValue,
        calibData.blueReference.raw.Y / maxSensorValue,
        calibData.blueReference.raw.Z / maxSensorValue
    );
    
    yellowPoint = Point3D(
        calibData.yellowReference.raw.X / maxSensorValue,
        calibData.yellowReference.raw.Y / maxSensorValue,
        calibData.yellowReference.raw.Z / maxSensorValue
    );
    
    // Set target RGB values for each reference point
    blackRGB = RGBColor(0, 0, 0);           // Pure black
    whiteRGB = RGBColor(255, 255, 255);     // Pure white
    blueRGB = RGBColor(0, 0, 255);          // Pure blue
    yellowRGB = RGBColor(255, 255, 0);      // Pure yellow
    
    // Validate tetrahedron geometry
    isValidTetrahedron = validateTetrahedronGeometry();
    
    if (isValidTetrahedron) {
        isInitialized = true;
        Serial.println("Tetrahedral interpolator initialized successfully");
        Serial.println("Reference points:");
        Serial.println("  Black: " + blackPoint.toString());
        Serial.println("  White: " + whitePoint.toString());
        Serial.println("  Blue: " + bluePoint.toString());
        Serial.println("  Yellow: " + yellowPoint.toString());
    } else {
        Serial.println("ERROR: Invalid tetrahedron geometry");
        isInitialized = false;
    }
    
    return isInitialized;
}

// Calculate 3x3 matrix determinant
float TetrahedralInterpolator::calculateDeterminant3x3(const float matrix[3][3]) const {
    return matrix[0][0] * (matrix[1][1] * matrix[2][2] - matrix[1][2] * matrix[2][1]) -
           matrix[0][1] * (matrix[1][0] * matrix[2][2] - matrix[1][2] * matrix[2][0]) +
           matrix[0][2] * (matrix[1][0] * matrix[2][1] - matrix[1][1] * matrix[2][0]);
}

// Calculate determinant with column replacement (Cramer's rule)
float TetrahedralInterpolator::calculateDeterminantWithColumn(const float matrix[3][3], 
                                                             const float rhs[3], int col) const {
    float tempMatrix[3][3];
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            tempMatrix[i][j] = (j == col) ? rhs[i] : matrix[i][j];
        }
    }
    return calculateDeterminant3x3(tempMatrix);
}

// Calculate barycentric coordinates using Cramer's rule
TetrahedralWeights TetrahedralInterpolator::calculateBarycentricWeights(const Point3D& p) const {
    TetrahedralWeights weights;
    
    // Create matrix for barycentric coordinate calculation
    // [x1-x4  x2-x4  x3-x4] [w1]   [x-x4]
    // [y1-y4  y2-y4  y3-y4] [w2] = [y-y4]
    // [z1-z4  z2-z4  z3-z4] [w3]   [z-z4]
    // where points are: 1=black, 2=white, 3=blue, 4=yellow
    
    float matrix[3][3] = {
        {blackPoint.x - yellowPoint.x, whitePoint.x - yellowPoint.x, bluePoint.x - yellowPoint.x},
        {blackPoint.y - yellowPoint.y, whitePoint.y - yellowPoint.y, bluePoint.y - yellowPoint.y},
        {blackPoint.z - yellowPoint.z, whitePoint.z - yellowPoint.z, bluePoint.z - yellowPoint.z}
    };
    
    float rhs[3] = {
        p.x - yellowPoint.x,
        p.y - yellowPoint.y,
        p.z - yellowPoint.z
    };
    
    // Calculate determinant
    float det = calculateDeterminant3x3(matrix);
    
    if (abs(det) < 0.0001f) {
        // Degenerate case - points are coplanar, fall back to triangular interpolation
        return calculateTriangularFallback(p);
    }
    
    // Calculate weights using Cramer's rule
    weights.black = calculateDeterminantWithColumn(matrix, rhs, 0) / det;
    weights.white = calculateDeterminantWithColumn(matrix, rhs, 1) / det;
    weights.blue = calculateDeterminantWithColumn(matrix, rhs, 2) / det;
    weights.yellow = 1.0f - weights.black - weights.white - weights.blue;
    
    // Check if point is inside tetrahedron
    if (weights.isInsideTetrahedron()) {
        weights.isValid = true;
    } else {
        // Point is outside tetrahedron, use distance-weighted interpolation
        weights = calculateDistanceWeightedFallback(p);
        fallbackCount++;
    }
    
    return weights;
}

// Fallback to triangular interpolation for degenerate cases
TetrahedralWeights TetrahedralInterpolator::calculateTriangularFallback(const Point3D& p) const {
    TetrahedralWeights weights;
    
    // Choose triangle based on which reference point is farthest from the query point
    float distToBlack = p.distanceTo(blackPoint);
    float distToWhite = p.distanceTo(whitePoint);
    float distToBlue = p.distanceTo(bluePoint);
    float distToYellow = p.distanceTo(yellowPoint);
    
    if (distToBlack >= max({distToWhite, distToBlue, distToYellow})) {
        // Use white-blue-yellow triangle
        weights = calculateTriangleWeights(p, whitePoint, bluePoint, yellowPoint);
        weights.black = 0;
    } else if (distToWhite >= max({distToBlue, distToYellow})) {
        // Use black-blue-yellow triangle
        weights = calculateTriangleWeights(p, blackPoint, bluePoint, yellowPoint);
        weights.white = 0;
    } else if (distToBlue >= distToYellow) {
        // Use black-white-yellow triangle
        weights = calculateTriangleWeights(p, blackPoint, whitePoint, yellowPoint);
        weights.blue = 0;
    } else {
        // Use black-white-blue triangle
        weights = calculateTriangleWeights(p, blackPoint, whitePoint, bluePoint);
        weights.yellow = 0;
    }
    
    weights.isValid = true;
    return weights;
}

// Distance-weighted interpolation for out-of-gamut points
TetrahedralWeights TetrahedralInterpolator::calculateDistanceWeightedFallback(const Point3D& p) const {
    TetrahedralWeights weights;
    
    // Calculate inverse distance weights
    float distToBlack = 1.0f / (p.distanceTo(blackPoint) + 0.001f);
    float distToWhite = 1.0f / (p.distanceTo(whitePoint) + 0.001f);
    float distToBlue = 1.0f / (p.distanceTo(bluePoint) + 0.001f);
    float distToYellow = 1.0f / (p.distanceTo(yellowPoint) + 0.001f);
    
    weights.black = distToBlack;
    weights.white = distToWhite;
    weights.blue = distToBlue;
    weights.yellow = distToYellow;
    
    weights.normalize();
    return weights;
}

// Calculate triangle weights using barycentric coordinates (simplified implementation)
TetrahedralWeights TetrahedralInterpolator::calculateTriangleWeights(const Point3D& p, const Point3D& p1, 
                                                                    const Point3D& p2, const Point3D& p3) const {
    TetrahedralWeights weights;
    
    // Simplified triangular interpolation using distance weighting
    float dist1 = 1.0f / (p.distanceTo(p1) + 0.001f);
    float dist2 = 1.0f / (p.distanceTo(p2) + 0.001f);
    float dist3 = 1.0f / (p.distanceTo(p3) + 0.001f);
    
    float totalDist = dist1 + dist2 + dist3;
    
    // Assign weights based on which points were used
    if (&p1 == &blackPoint) weights.black = dist1 / totalDist;
    else if (&p1 == &whitePoint) weights.white = dist1 / totalDist;
    else if (&p1 == &bluePoint) weights.blue = dist1 / totalDist;
    else if (&p1 == &yellowPoint) weights.yellow = dist1 / totalDist;
    
    if (&p2 == &blackPoint) weights.black = dist2 / totalDist;
    else if (&p2 == &whitePoint) weights.white = dist2 / totalDist;
    else if (&p2 == &bluePoint) weights.blue = dist2 / totalDist;
    else if (&p2 == &yellowPoint) weights.yellow = dist2 / totalDist;
    
    if (&p3 == &blackPoint) weights.black = dist3 / totalDist;
    else if (&p3 == &whitePoint) weights.white = dist3 / totalDist;
    else if (&p3 == &bluePoint) weights.blue = dist3 / totalDist;
    else if (&p3 == &yellowPoint) weights.yellow = dist3 / totalDist;
    
    weights.isValid = true;
    return weights;
}

// Validate tetrahedron geometry
bool TetrahedralInterpolator::validateTetrahedronGeometry() {
    // Check that points are not coplanar by calculating volume
    Point3D v1 = whitePoint - blackPoint;
    Point3D v2 = bluePoint - blackPoint;
    Point3D v3 = yellowPoint - blackPoint;
    
    // Calculate scalar triple product (volume * 6)
    float volume = abs(v1.x * (v2.y * v3.z - v2.z * v3.y) +
                      v1.y * (v2.z * v3.x - v2.x * v3.z) +
                      v1.z * (v2.x * v3.y - v2.y * v3.x));
    
    Serial.println("Tetrahedron volume: " + String(volume, 6));
    
    // Volume should be > 0 for non-coplanar points
    return volume > 0.000001f;
}

// Perform tetrahedral interpolation
TetrahedralWeights TetrahedralInterpolator::interpolate(uint16_t X, uint16_t Y, uint16_t Z) {
    TetrahedralWeights weights;

    if (!isInitialized) {
        Serial.println("ERROR: Interpolator not initialized");
        return weights;
    }

    interpolationCount++;

    // Normalize input point to [0,1] range
    Point3D queryPoint(X / 65535.0f, Y / 65535.0f, Z / 65535.0f);

    // Calculate barycentric weights
    weights = calculateBarycentricWeights(queryPoint);

    return weights;
}

// Convert XYZ to RGB using tetrahedral interpolation
bool TetrahedralInterpolator::convertXYZtoRGB(uint16_t X, uint16_t Y, uint16_t Z,
                                             uint8_t& R, uint8_t& G, uint8_t& B) {
    if (!isReady()) {
        return false;
    }

    // Get interpolation weights
    TetrahedralWeights weights = interpolate(X, Y, Z);

    if (!weights.isValid) {
        return false;
    }

    // Interpolate RGB values using weights
    float interpolatedR = weights.black * blackRGB.r +
                         weights.white * whiteRGB.r +
                         weights.blue * blueRGB.r +
                         weights.yellow * yellowRGB.r;

    float interpolatedG = weights.black * blackRGB.g +
                         weights.white * whiteRGB.g +
                         weights.blue * blueRGB.g +
                         weights.yellow * yellowRGB.g;

    float interpolatedB = weights.black * blackRGB.b +
                         weights.white * whiteRGB.b +
                         weights.blue * blueRGB.b +
                         weights.yellow * yellowRGB.b;

    // Convert to 8-bit RGB with bounds checking
    R = static_cast<uint8_t>(constrain(interpolatedR, 0, 255));
    G = static_cast<uint8_t>(constrain(interpolatedG, 0, 255));
    B = static_cast<uint8_t>(constrain(interpolatedB, 0, 255));

    return true;
}

// Get performance statistics
void TetrahedralInterpolator::getStatistics(uint32_t& totalInterpolations, uint32_t& fallbackUsed,
                                           float& fallbackRate) const {
    totalInterpolations = interpolationCount;
    fallbackUsed = fallbackCount;
    fallbackRate = interpolationCount > 0 ? (float)fallbackCount / interpolationCount * 100.0f : 0.0f;
}

// Reset performance counters
void TetrahedralInterpolator::resetStatistics() {
    interpolationCount = 0;
    fallbackCount = 0;
}

// Get debug information about reference points
String TetrahedralInterpolator::getDebugInfo() const {
    String info = "=== Tetrahedral Interpolator Debug Info ===\n";
    info += "Initialized: " + String(isInitialized ? "Yes" : "No") + "\n";
    info += "Valid Tetrahedron: " + String(isValidTetrahedron ? "Yes" : "No") + "\n";
    info += "Interpolations: " + String(interpolationCount) + "\n";
    info += "Fallbacks: " + String(fallbackCount) + "\n";

    if (isInitialized) {
        info += "Reference Points (normalized):\n";
        info += "  Black: " + blackPoint.toString() + " -> RGB(" + String(blackRGB.r) + "," + String(blackRGB.g) + "," + String(blackRGB.b) + ")\n";
        info += "  White: " + whitePoint.toString() + " -> RGB(" + String(whiteRGB.r) + "," + String(whiteRGB.g) + "," + String(whiteRGB.b) + ")\n";
        info += "  Blue: " + bluePoint.toString() + " -> RGB(" + String(blueRGB.r) + "," + String(blueRGB.g) + "," + String(blueRGB.b) + ")\n";
        info += "  Yellow: " + yellowPoint.toString() + " -> RGB(" + String(yellowRGB.r) + "," + String(yellowRGB.g) + "," + String(yellowRGB.b) + ")\n";
    }

    return info;
}

// Validate interpolation with test point
float TetrahedralInterpolator::validateInterpolation(uint16_t testX, uint16_t testY, uint16_t testZ,
                                                    uint8_t expectedR, uint8_t expectedG, uint8_t expectedB) {
    uint8_t actualR, actualG, actualB;

    if (!convertXYZtoRGB(testX, testY, testZ, actualR, actualG, actualB)) {
        return 999.0f; // Large error for failed conversion
    }

    // Calculate simple color difference (could be enhanced with CIEDE2000)
    float deltaR = actualR - expectedR;
    float deltaG = actualG - expectedG;
    float deltaB = actualB - expectedB;

    return sqrt(deltaR * deltaR + deltaG * deltaG + deltaB * deltaB);
}
