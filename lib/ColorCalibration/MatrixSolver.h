/**
 * @file MatrixSolver.h
 * @brief Least-squares matrix solver for 3x3 Color Correction Matrix calculation
 * @author ESP32 Color Calibration System
 * @version 1.0.0
 * @date 2024
 * 
 * This file provides the mathematical foundation for calculating the 3x3
 * Color Correction Matrix using least-squares approximation from calibration
 * data points.
 */

#ifndef MATRIX_SOLVER_H
#define MATRIX_SOLVER_H

#include "CalibrationStructures.h"
#include <Arduino.h>

/**
 * @brief Matrix solver class for Color Correction Matrix calculation
 * 
 * Implements least-squares approximation to calculate the optimal 3x3
 * transformation matrix for converting raw XYZ sensor readings to
 * corrected RGB values.
 */
class MatrixSolver {
public:
    /**
     * @brief Constructor
     */
    MatrixSolver();
    
    /**
     * @brief Destructor
     */
    ~MatrixSolver();
    
    /**
     * @brief Calculate Color Correction Matrix from calibration points
     * @param points Vector of calibration points
     * @param ccm Output color correction matrix
     * @return true if calculation successful, false otherwise
     */
    bool calculateCCM(const std::vector<CalibrationPoint>& points, ColorCorrectionMatrix& ccm);
    
    /**
     * @brief Validate calibration points for matrix calculation
     * @param points Vector of calibration points to validate
     * @return true if points are valid, false otherwise
     */
    bool validateCalibrationPoints(const std::vector<CalibrationPoint>& points);

    /**
     * @brief Check color diversity to prevent singular matrix
     * @param points Vector of calibration points to check
     * @return true if sufficient diversity, false otherwise
     */
    bool checkColorDiversity(const std::vector<CalibrationPoint>& points);
    
    /**
     * @brief Get last error message
     * @return Error message string
     */
    String getLastError() const { return lastError; }

private:
    String lastError; ///< Last error message
    
    /**
     * @brief Solve least-squares for a single RGB channel
     * @param points Calibration points
     * @param channel 0=red, 1=green, 2=blue
     * @param coefficients Output coefficients [3]
     * @return true if successful, false otherwise
     */
    bool solveChannel(const std::vector<CalibrationPoint>& points, uint8_t channel, float coefficients[3]);
    
    /**
     * @brief Invert 3x3 matrix using analytical method
     * @param matrix Input matrix [3][3]
     * @param inverse Output inverse matrix [3][3]
     * @return true if inversion successful, false otherwise
     */
    bool invert3x3(const float matrix[3][3], float inverse[3][3]);
    
    /**
     * @brief Calculate matrix determinant
     * @param matrix Input matrix [3][3]
     * @return Determinant value
     */
    float determinant3x3(const float matrix[3][3]);
    
    /**
     * @brief Calculate matrix condition number
     * @param matrix Input matrix [3][3]
     * @return Condition number
     */
    float conditionNumber3x3(const float matrix[3][3]);
    
    /**
     * @brief Normalize XYZ values to 0-1 range
     * @param x Raw X value
     * @param y Raw Y value
     * @param z Raw Z value
     * @param nx Normalized X output
     * @param ny Normalized Y output
     * @param nz Normalized Z output
     */
    void normalizeXYZ(uint16_t x, uint16_t y, uint16_t z, float& nx, float& ny, float& nz);
};

#endif // MATRIX_SOLVER_H
