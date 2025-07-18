/*
 * CIEDE2000.h
 * Part of http://github.com/gfiumara/CIEDE2000 by Gregory Fiumara.
 * See LICENSE for details.
 */

#ifndef GPF_CIEDE2000_H_
#define GPF_CIEDE2000_H_

#include <iostream>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288 /* pi */
#endif

/** Namespace containing all necessary objects and methods for CIEDE2000 */
namespace CIEDE2000
{

/*****************************************************************************
 * Types.
 *****************************************************************************/

/** A color in CIELAB colorspace */
struct LAB
{
	/** Lightness */
	double l;
	/** Color-opponent a dimension */
	double a;
	/** Color-opponent b dimension */
	double b;
};

/** Convenience definition for struct LAB */
using LAB = struct LAB;

/*****************************************************************************
 * Operations.
 *****************************************************************************/

/**
 * @brief
 * Obtain Delta-E 2000 value.
 * @details
 * Based on the paper "The CIEDE2000 Color-Difference Formula:
 * Implementation Notes, Supplementary Test Data, and Mathematical
 * Observations" by Gaurav Sharma, Wencheng Wu, and Edul N. Dalal,
 * from http://www.ece.rochester.edu/~gsharma/ciede2000/.
 *
 * @param lab1
 * First color in LAB colorspace.
 * @param lab2
 * Second color in LAB colorspace.
 *
 * @return
 * Delta-E difference between lab1 and lab2.
 */
double CIEDE2000(
	const LAB &lab1,
	const LAB &lab2);

/*****************************************************************************
 * Conversions.
 *****************************************************************************/

/**
 * @brief
 * Convert degrees to radians.
 *
 * @param deg
 * Angle in degrees.
 *
 * @return
 * deg in radians.
 */
constexpr double deg2Rad(
	const double deg);

/**
 * @brief
 * Convert radians to degrees.
 *
 * @param rad
 * Angle in radians.
 *
 * @return
 * rad in degrees.
 */
constexpr double rad2Deg(
	const double rad);

}

/*****************************************************************************
 * RGB to LAB conversion functions for ESP32 color matching
 *****************************************************************************/

/**
 * @brief
 * Convert RGB to XYZ colorspace (D65 illuminant)
 * 
 * @param r Red component (0-255)
 * @param g Green component (0-255) 
 * @param b Blue component (0-255)
 * @param x Output X component
 * @param y Output Y component
 * @param z Output Z component
 */
void rgbToXYZ(uint8_t r, uint8_t g, uint8_t b, double &x, double &y, double &z);

/**
 * @brief
 * Convert XYZ to LAB colorspace (D65 illuminant)
 * 
 * @param x X component
 * @param y Y component
 * @param z Z component
 * @param lab Output LAB color
 */
void xyzToLAB(double x, double y, double z, CIEDE2000::LAB &lab);

/**
 * @brief
 * Convert RGB directly to LAB colorspace
 * 
 * @param r Red component (0-255)
 * @param g Green component (0-255)
 * @param b Blue component (0-255)
 * @param lab Output LAB color
 */
void rgbToLAB(uint8_t r, uint8_t g, uint8_t b, CIEDE2000::LAB &lab);

/*****************************************************************************
 * Conversions.
 *****************************************************************************/

/**
 * @brief
 * LAB output stream operator.
 *
 * @param s
 * Output stream.
 * @param labColor
 * Color to output.
 *
 * @return
 * s with labColor appended.
 */
std::ostream& operator<<(
	std::ostream &s,
	const CIEDE2000::LAB &labColor);

#endif /* GPF_CIEDE2000_H_ */
