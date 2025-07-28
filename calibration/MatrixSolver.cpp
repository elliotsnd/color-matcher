/*!
 * @file MatrixSolver.cpp
 * @brief Implementation of Least-Squares Matrix Solver
 * @copyright ESP32 Color Matcher Project
 * @version 3.0
 * @date 2024
 */

#include "MatrixSolver.h"
#include <cmath>

namespace ColorCalibration {

ColorCorrectionMatrix MatrixSolver::calculateCCM(const std::vector<CalibrationPoint>& points) {
    ColorCorrectionMatrix ccm;
    
    // Validate input points
    if (!validateCalibrationPoints(points)) {
        Serial.println("ERROR: Invalid calibration points for CCM calculation");
        return ccm; // Returns invalid matrix
    }
    
    Serial.println("Calculating Color Correction Matrix from " + String(points.size()) + " calibration points...");
    
    // Solve for each RGB channel separately using least-squares
    float rCoefficients[3], gCoefficients[3], bCoefficients[3];
    
    bool rSuccess = solveChannel(points, 0, rCoefficients); // Red channel
    bool gSuccess = solveChannel(points, 1, gCoefficients); // Green channel  
    bool bSuccess = solveChannel(points, 2, bCoefficients); // Blue channel
    
    if (!rSuccess || !gSuccess || !bSuccess) {
        Serial.println("ERROR: Failed to solve for one or more RGB channels");
        return ccm; // Returns invalid matrix
    }
    
    // Populate the Color Correction Matrix
    // Row 0: Red channel coefficients
    ccm.m[0][0] = rCoefficients[0]; // X coefficient for Red
    ccm.m[0][1] = rCoefficients[1]; // Y coefficient for Red
    ccm.m[0][2] = rCoefficients[2]; // Z coefficient for Red
    
    // Row 1: Green channel coefficients  
    ccm.m[1][0] = gCoefficients[0]; // X coefficient for Green
    ccm.m[1][1] = gCoefficients[1]; // Y coefficient for Green
    ccm.m[1][2] = gCoefficients[2]; // Z coefficient for Green
    
    // Row 2: Blue channel coefficients
    ccm.m[2][0] = bCoefficients[0]; // X coefficient for Blue
    ccm.m[2][1] = bCoefficients[1]; // Y coefficient for Blue
    ccm.m[2][2] = bCoefficients[2]; // Z coefficient for Blue
    
    // Calculate matrix quality metrics
    ccm.calculateDeterminant();
    ccm.conditionNumber = calculateConditionNumber(ccm.m);
    
    if (ccm.isValid) {
        Serial.println("✅ Color Correction Matrix calculated successfully!");
        Serial.println("   Determinant: " + String(ccm.determinant, 6));
        Serial.println("   Condition Number: " + String(ccm.conditionNumber, 2));
        
        // Log the matrix for debugging
        Serial.println("   Matrix:");
        for (int i = 0; i < 3; i++) {
            Serial.println("   [" + String(ccm.m[i][0], 4) + ", " + 
                          String(ccm.m[i][1], 4) + ", " + 
                          String(ccm.m[i][2], 4) + "]");
        }
    } else {
        Serial.println("❌ Color Correction Matrix is invalid (singular matrix)");
    }
    
    return ccm;
}
bool MatrixSolver::solveChannel(const std::vector<CalibrationPoint>& points, 
                               int channelIndex, float coefficients[3]) {
    const int n = points.size(); // Number of calibration points
    
    // Build the A matrix (n x 3) and b vector (n x 1) for least-squares
    // A contains normalized XYZ values, b contains target RGB values for this channel
    float A[n][3];  // Matrix of raw XYZ readings
    float b[n];     // Vector of target RGB values for this channel
    
    for (int i = 0; i < n; i++) {
        // Normalize raw XYZ values to 0-1 range
        A[i][0] = static_cast<float>(points[i].rawX) / 65535.0f;
        A[i][1] = static_cast<float>(points[i].rawY) / 65535.0f;
        A[i][2] = static_cast<float>(points[i].rawZ) / 65535.0f;
        
        // Get target value for this RGB channel (normalized to 0-1)
        switch (channelIndex) {
            case 0: b[i] = static_cast<float>(points[i].targetR) / 255.0f; break; // Red
            case 1: b[i] = static_cast<float>(points[i].targetG) / 255.0f; break; // Green
            case 2: b[i] = static_cast<float>(points[i].targetB) / 255.0f; break; // Blue
            default: return false;
        }
    }
    
    // Calculate A^T * A (3x3 matrix)
    float AtA[3][3];
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            AtA[i][j] = 0.0f;
            for (int k = 0; k < n; k++) {
                AtA[i][j] += A[k][i] * A[k][j];
            }
        }
    }
    
    // Calculate A^T * b (3x1 vector)
    float Atb[3];
    for (int i = 0; i < 3; i++) {
        Atb[i] = 0.0f;
        for (int k = 0; k < n; k++) {
            Atb[i] += A[k][i] * b[k];
        }
    }
    
    // Solve (A^T * A) * coefficients = A^T * b
    // This requires inverting the 3x3 matrix A^T * A
    float AtA_inv[3][3];
    if (!invert3x3(AtA, AtA_inv)) {
        Serial.println("ERROR: Cannot invert A^T*A matrix for channel " + String(channelIndex));
        return false;
    }
    
    // Calculate coefficients = (A^T * A)^-1 * A^T * b
    for (int i = 0; i < 3; i++) {
        coefficients[i] = 0.0f;
        for (int j = 0; j < 3; j++) {
            coefficients[i] += AtA_inv[i][j] * Atb[j];
        }
    }
    
    return true;
}bool MatrixSolver::invert3x3(const float matrix[3][3], float inverse[3][3]) {
    // Calculate determinant first
    float det = calculateDeterminant(matrix);
    
    // Check if matrix is singular (non-invertible)
    // CRITICAL FIX: Use appropriate epsilon for float precision
    // 1e-9 was too small for 32-bit float, causing false negatives
    // 1e-6 is appropriate for single-precision floating point
    if (abs(det) < 1e-6f) {
        Serial.println("ERROR: Matrix is singular (determinant ≈ 0), cannot invert");
        return false;
    }
    
    // Calculate adjugate matrix (cofactor matrix transposed)
    float adj[3][3];
    
    // Row 0 of adjugate
    adj[0][0] = matrix[1][1] * matrix[2][2] - matrix[1][2] * matrix[2][1];
    adj[0][1] = matrix[0][2] * matrix[2][1] - matrix[0][1] * matrix[2][2];
    adj[0][2] = matrix[0][1] * matrix[1][2] - matrix[0][2] * matrix[1][1];
    
    // Row 1 of adjugate
    adj[1][0] = matrix[1][2] * matrix[2][0] - matrix[1][0] * matrix[2][2];
    adj[1][1] = matrix[0][0] * matrix[2][2] - matrix[0][2] * matrix[2][0];
    adj[1][2] = matrix[0][2] * matrix[1][0] - matrix[0][0] * matrix[1][2];
    
    // Row 2 of adjugate
    adj[2][0] = matrix[1][0] * matrix[2][1] - matrix[1][1] * matrix[2][0];
    adj[2][1] = matrix[0][1] * matrix[2][0] - matrix[0][0] * matrix[2][1];
    adj[2][2] = matrix[0][0] * matrix[1][1] - matrix[0][1] * matrix[1][0];
    
    // Calculate inverse: inverse = adjugate / determinant
    float invDet = 1.0f / det;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            inverse[i][j] = adj[i][j] * invDet;
        }
    }
    
    return true;
}

float MatrixSolver::calculateDeterminant(const float matrix[3][3]) {
    return matrix[0][0] * (matrix[1][1] * matrix[2][2] - matrix[1][2] * matrix[2][1]) -
           matrix[0][1] * (matrix[1][0] * matrix[2][2] - matrix[1][2] * matrix[2][0]) +
           matrix[0][2] * (matrix[1][0] * matrix[2][1] - matrix[1][1] * matrix[2][0]);
}

bool MatrixSolver::validateCalibrationPoints(const std::vector<CalibrationPoint>& points) {
    // Need at least 3 points for a 3x3 matrix solution
    if (points.size() < 3) {
        Serial.println("ERROR: Need at least 3 calibration points, got " + String(points.size()));
        return false;
    }
    
    // Check for duplicate or invalid points
    for (size_t i = 0; i < points.size(); i++) {
        const auto& point = points[i];
        
        // Check for zero readings (likely invalid)
        if (point.rawX == 0 && point.rawY == 0 && point.rawZ == 0) {
            Serial.println("ERROR: Calibration point " + String(i) + " has zero XYZ readings");
            return false;
        }
        
        // Check for duplicate points (same raw XYZ values)
        for (size_t j = i + 1; j < points.size(); j++) {
            const auto& other = points[j];
            if (point.rawX == other.rawX && point.rawY == other.rawY && point.rawZ == other.rawZ) {
                Serial.println("WARNING: Duplicate calibration points detected at indices " + 
                              String(i) + " and " + String(j));
            }
        }
    }
    
    return true;
}float MatrixSolver::calculateConditionNumber(const float matrix[3][3]) {
    // Simplified condition number estimation
    // In a full implementation, this would calculate the ratio of largest to smallest singular values
    // For now, we use a simpler metric based on the determinant and matrix norms
    
    float det = abs(calculateDeterminant(matrix));
    // CRITICAL FIX: Use appropriate epsilon for float precision
    if (det < 1e-6f) {
        return 1e9f; // Very high condition number for near-singular matrices
    }
    
    // Calculate Frobenius norm of the matrix
    float frobeniusNorm = 0.0f;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            frobeniusNorm += matrix[i][j] * matrix[i][j];
        }
    }
    frobeniusNorm = sqrt(frobeniusNorm);
    
    // Simple condition number estimate
    // Lower values indicate better-conditioned matrices
    float conditionNumber = frobeniusNorm / det;
    
    return conditionNumber;
}

} // namespace ColorCalibration
          