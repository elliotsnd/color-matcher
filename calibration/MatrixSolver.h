[Reading 81 lines from start (total: 81 lines, 0 remaining)]

/*!
 * @file MatrixSolver.h
 * @brief Least-Squares Matrix Solver for 5-Point Color Correction Matrix
 * @copyright ESP32 Color Matcher Project
 * @version 3.0
 * @date 2024
 * 
 * Implements the mathematical core for calculating the Color Correction Matrix
 * from calibration points using least-squares approximation. This replaces
 * the flawed local-patch approach with industry-standard matrix mathematics.
 */

#pragma once

#include "CalibrationStructures.h"
#include "Arduino.h"
#include <vector>

namespace ColorCalibration {

/**
 * @brief Matrix solver for Color Correction Matrix calculation
 * 
 * This class implements the least-squares solution for finding the optimal
 * 3x3 Color Correction Matrix from a set of calibration points.
 * 
 * Mathematical approach:
 * For each RGB channel, we solve: coefficients = (A^T * A)^-1 * A^T * b
 * where A is the matrix of raw XYZ readings and b is the target RGB values.
 */
class MatrixSolver {
public:
    /**
     * @brief Calculate Color Correction Matrix from calibration points
     * @param points Vector of calibration points (minimum 3, optimal 5)
     * @return Calculated Color Correction Matrix
     */
    static ColorCorrectionMatrix calculateCCM(const std::vector<CalibrationPoint>& points);

private:
    /**
     * @brief Invert a 3x3 matrix using analytical method
     * @param matrix Input 3x3 matrix (row-major order)
     * @param inverse Output inverted matrix
     * @return true if inversion successful, false if matrix is singular
     */
    static bool invert3x3(const float matrix[3][3], float inverse[3][3]);
    
    /**
     * @brief Solve for one RGB channel using least-squares
     * @param points Calibration points
     * @param channelIndex RGB channel index (0=R, 1=G, 2=B)
     * @param coefficients Output coefficients for this channel
     * @return true if solution found
     */
    static bool solveChannel(const std::vector<CalibrationPoint>& points, 
                           int channelIndex, float coefficients[3]);
    
    /**
     * @brief Calculate matrix determinant
     * @param matrix 3x3 matrix
     * @return Determinant value
     */
    static float calculateDeterminant(const float matrix[3][3]);
    
    /**
     * @brief Validate calibration points for matrix calculation
     * @param points Calibration points to validate
     * @return true if points are suitable for matrix calculation
     */
    static bool validateCalibrationPoints(const std::vector<CalibrationPoint>& points);
    
    /**
     * @brief Calculate condition number for matrix stability assessment
     * @param matrix 3x3 matrix
     * @return Condition number (lower is better)
     */
    static float calculateConditionNumber(const float matrix[3][3]);
};

} // namespace ColorCalibration
          