/**
 * @file MatrixSolver.cpp
 * @brief Implementation of least-squares matrix solver for Color Correction Matrix
 * @author ESP32 Color Calibration System
 * @version 1.0.0
 * @date 2024
 * 
 * This file contains the complete implementation of the mathematical algorithms
 * required to calculate the optimal 3x3 Color Correction Matrix using
 * least-squares approximation from calibration data.
 */

#include "MatrixSolver.h"
#include <cmath>

MatrixSolver::MatrixSolver() {
    lastError = "";
}

MatrixSolver::~MatrixSolver() {
    // Nothing to clean up
}

bool MatrixSolver::calculateCCM(const std::vector<CalibrationPoint>& points, ColorCorrectionMatrix& ccm) {
    lastError = "";

    // === ENHANCED ERROR HANDLING AND VALIDATION ===

    Serial.println("🔬 MatrixSolver: Starting CCM calculation...");
    Serial.println("   Input points: " + String(points.size()));

    // Guard clause: Check for null/empty input
    if (points.empty()) {
        lastError = "No calibration points provided";
        Serial.println("❌ MatrixSolver: Empty calibration points vector");
        ccm.isValid = false;
        return false;
    }

    // Guard clause: Validate minimum points for matrix calculation
    if (points.size() < 3) {
        lastError = "Need at least 3 calibration points for 3x3 matrix calculation (provided: " + String(points.size()) + ")";
        Serial.println("❌ MatrixSolver: Insufficient calibration points (" + String(points.size()) + "/3 minimum)");
        ccm.isValid = false;
        return false;
    }

    // Enhanced validation with detailed error reporting
    if (!validateCalibrationPoints(points)) {
        Serial.println("❌ MatrixSolver: Calibration point validation FAILED");
        Serial.println("   Reason: " + lastError);
        ccm.isValid = false;
        return false;
    }

    // Guard clause: Check for color diversity (prevent singular matrix)
    if (!checkColorDiversity(points)) {
        lastError = "Insufficient color diversity in calibration points - matrix may be singular";
        Serial.println("❌ MatrixSolver: " + lastError);
        Serial.println("   Suggestion: Use more diverse colors (different RGB values)");
        ccm.isValid = false;
        return false;
    }

    Serial.println("✅ MatrixSolver: Point validation passed");

    // Solve for each RGB channel
    float redCoeffs[3], greenCoeffs[3], blueCoeffs[3];

    Serial.println("🔴 Solving RED channel...");
    if (!solveChannel(points, 0, redCoeffs)) {
        lastError = "Failed to solve red channel: " + lastError;
        Serial.println("❌ CCM FAIL: Failed to solve for Red channel.");
        Serial.println("   Reason: Failed to invert matrix. The calibration points are likely not diverse enough.");
        Serial.println("   Suggestion: Ensure your palette includes strong primary colors (Red, Green, Blue) in addition to neutrals.");
        return false;
    }
    Serial.println("✅ RED channel solved: [" + String(redCoeffs[0], 6) + ", " + String(redCoeffs[1], 6) + ", " + String(redCoeffs[2], 6) + "]");

    Serial.println("🟢 Solving GREEN channel...");
    if (!solveChannel(points, 1, greenCoeffs)) {
        lastError = "Failed to solve green channel: " + lastError;
        Serial.println("❌ CCM FAIL: Failed to solve for Green channel.");
        Serial.println("   Reason: Failed to invert matrix. The calibration points are likely not diverse enough.");
        Serial.println("   Suggestion: Ensure your palette includes strong primary colors (Red, Green, Blue) in addition to neutrals.");
        return false;
    }
    Serial.println("✅ GREEN channel solved: [" + String(greenCoeffs[0], 6) + ", " + String(greenCoeffs[1], 6) + ", " + String(greenCoeffs[2], 6) + "]");

    Serial.println("🔵 Solving BLUE channel...");
    if (!solveChannel(points, 2, blueCoeffs)) {
        lastError = "Failed to solve blue channel: " + lastError;
        Serial.println("❌ CCM FAIL: Failed to solve for Blue channel.");
        Serial.println("   Reason: Failed to invert matrix. The calibration points are likely not diverse enough.");
        Serial.println("   Suggestion: Ensure your palette includes strong primary colors (Red, Green, Blue) in addition to neutrals.");
        return false;
    }
    Serial.println("✅ BLUE channel solved: [" + String(blueCoeffs[0], 6) + ", " + String(blueCoeffs[1], 6) + ", " + String(blueCoeffs[2], 6) + "]");
    
    // Build the color correction matrix
    // Each row represents the coefficients for R, G, B respectively
    for (int i = 0; i < 3; i++) {
        ccm.m[0][i] = redCoeffs[i];    // Red channel coefficients
        ccm.m[1][i] = greenCoeffs[i];  // Green channel coefficients
        ccm.m[2][i] = blueCoeffs[i];   // Blue channel coefficients
    }
    
    // Calculate matrix properties
    ccm.determinant = determinant3x3(ccm.m);
    ccm.conditionNumber = conditionNumber3x3(ccm.m);
    
    // Validate matrix stability
    if (fabs(ccm.determinant) < 1e-6) {
        lastError = "Matrix is singular (determinant too small)";
        ccm.isValid = false;
        return false;
    }
    
    if (ccm.conditionNumber > 1000.0f) {
        lastError = "Matrix is ill-conditioned (condition number too high)";
        ccm.isValid = false;
        return false;
    }
    
    ccm.isValid = true;
    Serial.println("🎉 MatrixSolver: CCM calculation SUCCESSFUL!");
    Serial.println("   Matrix is now valid and ready for use");
    Serial.println("   Determinant: " + String(ccm.determinant, 6));
    Serial.println("   Condition Number: " + String(ccm.conditionNumber, 2));
    return true;
}

bool MatrixSolver::validateCalibrationPoints(const std::vector<CalibrationPoint>& points) {
    if (points.empty()) {
        lastError = "No calibration points provided";
        return false;
    }
    
    // Check for duplicate target colors (6-color system)
    bool seenBlack = false, seenWhite = false, seenRed = false, seenGreen = false, seenBlue = false, seenYellow = false;
    
    for (const auto& point : points) {
        // Validate sensor readings
        if (point.rawX == 0 && point.rawY == 0 && point.rawZ == 0) {
            lastError = "Invalid sensor reading (all zeros)";
            return false;
        }
        
        // Check for saturated readings (assuming 16-bit sensor)
        if (point.rawX >= 65535 || point.rawY >= 65535 || point.rawZ >= 65535) {
            lastError = "Sensor reading saturated";
            return false;
        }
        
        // Check for duplicate target colors
        if (point.targetR == TargetColors::BLACK_R && 
            point.targetG == TargetColors::BLACK_G && 
            point.targetB == TargetColors::BLACK_B) {
            if (seenBlack) {
                lastError = "Duplicate black calibration point";
                return false;
            }
            seenBlack = true;
        }
        
        if (point.targetR == TargetColors::WHITE_R && 
            point.targetG == TargetColors::WHITE_G && 
            point.targetB == TargetColors::WHITE_B) {
            if (seenWhite) {
                lastError = "Duplicate white calibration point";
                return false;
            }
            seenWhite = true;
        }
        
        if (point.targetR == TargetColors::RED_R &&
            point.targetG == TargetColors::RED_G &&
            point.targetB == TargetColors::RED_B) {
            if (seenRed) {
                lastError = "Duplicate red calibration point";
                return false;
            }
            seenRed = true;
        }

        if (point.targetR == TargetColors::GREEN_R &&
            point.targetG == TargetColors::GREEN_G &&
            point.targetB == TargetColors::GREEN_B) {
            if (seenGreen) {
                lastError = "Duplicate green calibration point";
                return false;
            }
            seenGreen = true;
        }

        if (point.targetR == TargetColors::BLUE_R &&
            point.targetG == TargetColors::BLUE_G &&
            point.targetB == TargetColors::BLUE_B) {
            if (seenBlue) {
                lastError = "Duplicate blue calibration point";
                return false;
            }
            seenBlue = true;
        }
        
        if (point.targetR == TargetColors::YELLOW_R && 
            point.targetG == TargetColors::YELLOW_G && 
            point.targetB == TargetColors::YELLOW_B) {
            if (seenYellow) {
                lastError = "Duplicate yellow calibration point";
                return false;
            }
            seenYellow = true;
        }
    }
    
    return true;
}

bool MatrixSolver::checkColorDiversity(const std::vector<CalibrationPoint>& points) {
    // === COLOR DIVERSITY VALIDATION FOR ROBUST MATRIX CALCULATION ===

    if (points.size() < 3) {
        return false; // Already checked elsewhere, but safety first
    }

    // Check for sufficient RGB range diversity
    uint8_t minR = 255, maxR = 0;
    uint8_t minG = 255, maxG = 0;
    uint8_t minB = 255, maxB = 0;

    for (const auto& point : points) {
        minR = min(minR, point.targetR);
        maxR = max(maxR, point.targetR);
        minG = min(minG, point.targetG);
        maxG = max(maxG, point.targetG);
        minB = min(minB, point.targetB);
        maxB = max(maxB, point.targetB);
    }

    // Check for minimum range in each channel (at least 50 units difference)
    const uint8_t MIN_RANGE = 50;
    if ((maxR - minR) < MIN_RANGE) {
        lastError = "Insufficient red channel diversity (range: " + String(maxR - minR) + ", minimum: " + String(MIN_RANGE) + ")";
        return false;
    }
    if ((maxG - minG) < MIN_RANGE) {
        lastError = "Insufficient green channel diversity (range: " + String(maxG - minG) + ", minimum: " + String(MIN_RANGE) + ")";
        return false;
    }
    if ((maxB - minB) < MIN_RANGE) {
        lastError = "Insufficient blue channel diversity (range: " + String(maxB - minB) + ", minimum: " + String(MIN_RANGE) + ")";
        return false;
    }

    // Check for sensor reading diversity (prevent all points being too similar)
    uint16_t minX = 65535, maxX = 0;
    uint16_t minY = 65535, maxY = 0;
    uint16_t minZ = 65535, maxZ = 0;

    for (const auto& point : points) {
        minX = min(minX, point.rawX);
        maxX = max(maxX, point.rawX);
        minY = min(minY, point.rawY);
        maxY = max(maxY, point.rawY);
        minZ = min(minZ, point.rawZ);
        maxZ = max(maxZ, point.rawZ);
    }

    // Check for minimum sensor range (at least 1000 units difference)
    const uint16_t MIN_SENSOR_RANGE = 1000;
    if ((maxX - minX) < MIN_SENSOR_RANGE || (maxY - minY) < MIN_SENSOR_RANGE || (maxZ - minZ) < MIN_SENSOR_RANGE) {
        lastError = "Insufficient sensor reading diversity - XYZ ranges: X(" + String(maxX - minX) + ") Y(" + String(maxY - minY) + ") Z(" + String(maxZ - minZ) + "), minimum: " + String(MIN_SENSOR_RANGE);
        return false;
    }

    Serial.println("✅ MatrixSolver: Color diversity check passed");
    Serial.println("   RGB ranges: R(" + String(maxR - minR) + ") G(" + String(maxG - minG) + ") B(" + String(maxB - minB) + ")");
    Serial.println("   XYZ ranges: X(" + String(maxX - minX) + ") Y(" + String(maxY - minY) + ") Z(" + String(maxZ - minZ) + ")");

    return true;
}

bool MatrixSolver::solveChannel(const std::vector<CalibrationPoint>& points, uint8_t channel, float coefficients[3]) {
    // Build the system of equations: A^T * A * x = A^T * b
    
    // A^T * A matrix (3x3)
    float AtA[3][3] = {{0}};
    
    // A^T * b vector (3x1)
    float Atb[3] = {0};
    
    // Build the matrices
    for (const auto& point : points) {
        float nx, ny, nz;
        normalizeXYZ(point.rawX, point.rawY, point.rawZ, nx, ny, nz);
        
        float target;
        switch (channel) {
            case 0: target = point.targetR / 255.0f; break;
            case 1: target = point.targetG / 255.0f; break;
            case 2: target = point.targetB / 255.0f; break;
            default: return false;
        }
        
        // Build A^T * A
        AtA[0][0] += nx * nx;
        AtA[0][1] += nx * ny;
        AtA[0][2] += nx * nz;
        AtA[1][0] += ny * nx;
        AtA[1][1] += ny * ny;
        AtA[1][2] += ny * nz;
        AtA[2][0] += nz * nx;
        AtA[2][1] += nz * ny;
        AtA[2][2] += nz * nz;
        
        // Build A^T * b
        Atb[0] += nx * target;
        Atb[1] += ny * target;
        Atb[2] += nz * target;
    }
    
    // Add regularization for underdetermined systems (fewer than 3 points)
    // This adds a small value to the diagonal to make the matrix invertible
    float regularization = 1e-6f;
    AtA[0][0] += regularization;
    AtA[1][1] += regularization;
    AtA[2][2] += regularization;

    // Invert A^T * A
    float invAtA[3][3];
    if (!invert3x3(AtA, invAtA)) {
        lastError = "Failed to invert A^T * A matrix";
        return false;
    }
    
    // Solve for coefficients: x = (A^T * A)^-1 * A^T * b
    coefficients[0] = invAtA[0][0] * Atb[0] + invAtA[0][1] * Atb[1] + invAtA[0][2] * Atb[2];
    coefficients[1] = invAtA[1][0] * Atb[0] + invAtA[1][1] * Atb[1] + invAtA[1][2] * Atb[2];
    coefficients[2] = invAtA[2][0] * Atb[0] + invAtA[2][1] * Atb[1] + invAtA[2][2] * Atb[2];
    
    return true;
}

bool MatrixSolver::invert3x3(const float matrix[3][3], float inverse[3][3]) {
    float det = determinant3x3(matrix);

    // CRITICAL FIX: Use appropriate epsilon for float precision
    // 1e-10 was too small for 32-bit float, causing false negatives
    // 1e-6 is appropriate for single-precision floating point
    if (fabs(det) < 1e-6f) {
        lastError = "Matrix is singular (determinant = 0)";
        return false;
    }
    
    float invDet = 1.0f / det;
    
    // Calculate cofactor matrix and transpose
    inverse[0][0] = (matrix[1][1] * matrix[2][2] - matrix[1][2] * matrix[2][1]) * invDet;
    inverse[0][1] = (matrix[0][2] * matrix[2][1] - matrix[0][1] * matrix[2][2]) * invDet;
    inverse[0][2] = (matrix[0][1] * matrix[1][2] - matrix[0][2] * matrix[1][1]) * invDet;
    
    inverse[1][0] = (matrix[1][2] * matrix[2][0] - matrix[1][0] * matrix[2][2]) * invDet;
    inverse[1][1] = (matrix[0][0] * matrix[2][2] - matrix[0][2] * matrix[2][0]) * invDet;
    inverse[1][2] = (matrix[0][2] * matrix[1][0] - matrix[0][0] * matrix[1][2]) * invDet;
    
    inverse[2][0] = (matrix[1][0] * matrix[2][1] - matrix[1][1] * matrix[2][0]) * invDet;
    inverse[2][1] = (matrix[0][1] * matrix[2][0] - matrix[0][0] * matrix[2][1]) * invDet;
    inverse[2][2] = (matrix[0][0] * matrix[1][1] - matrix[0][1] * matrix[1][0]) * invDet;
    
    return true;
}

float MatrixSolver::determinant3x3(const float matrix[3][3]) {
    return matrix[0][0] * (matrix[1][1] * matrix[2][2] - matrix[1][2] * matrix[2][1]) -
           matrix[0][1] * (matrix[1][0] * matrix[2][2] - matrix[1][2] * matrix[2][0]) +
           matrix[0][2] * (matrix[1][0] * matrix[2][1] - matrix[1][1] * matrix[2][0]);
}

float MatrixSolver::conditionNumber3x3(const float matrix[3][3]) {
    // Simple condition number estimation using Frobenius norm
    float norm = 0.0f;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            norm += matrix[i][j] * matrix[i][j];
        }
    }
    norm = sqrt(norm);
    
    float inverse[3][3];
    if (!invert3x3(matrix, inverse)) {
        return INFINITY;
    }
    
    float invNorm = 0.0f;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            invNorm += inverse[i][j] * inverse[i][j];
        }
    }
    invNorm = sqrt(invNorm);
    
    return norm * invNorm;
}

void MatrixSolver::normalizeXYZ(uint16_t x, uint16_t y, uint16_t z, float& nx, float& ny, float& nz) {
    // Normalize to 0-1 range based on 16-bit sensor range
    const float MAX_VALUE = 65535.0f;
    nx = (float)x / MAX_VALUE;
    ny = (float)y / MAX_VALUE;
    nz = (float)z / MAX_VALUE;
}


