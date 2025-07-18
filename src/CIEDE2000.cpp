/*
 * CIEDE2000.cpp
 * Part of http://github.com/gfiumara/CIEDE2000 by Gregory Fiumara.
 * See LICENSE for details.
 */

#include "CIEDE2000.h"
#include <cmath>

/*****************************************************************************
 * Conversions.
 *****************************************************************************/

constexpr double CIEDE2000::deg2Rad(
	const double deg)
{
	return (deg * (M_PI / 180.0));
}

constexpr double CIEDE2000::rad2Deg(
	const double rad)
{
	return ((180.0 / M_PI) * rad);
}

double CIEDE2000::CIEDE2000(
	const LAB &lab1,
	const LAB &lab2)
{
	/*
	 * "For these and all other numerical/graphical 􏰀delta E00 values
	 * reported in this article, we set the parametric weighting factors
	 * to unity(i.e., k_L = k_C = k_H = 1.0)." (Page 27).
	 */
	const double k_L = 1.0, k_C = 1.0, k_H = 1.0;
	const double deg360InRad = CIEDE2000::deg2Rad(360.0);
	const double deg180InRad = CIEDE2000::deg2Rad(180.0);
	const double pow25To7 = 6103515625.0; /* pow(25, 7) */

	/*
	 * Step 1
	 */
	/* Equation 2 */
	double C1 = sqrt((lab1.a * lab1.a) + (lab1.b * lab1.b));
	double C2 = sqrt((lab2.a * lab2.a) + (lab2.b * lab2.b));
	/* Equation 3 */
	double barC = (C1 + C2) / 2.0;
	/* Equation 4 */
	double G = 0.5 * (1 - sqrt(pow(barC, 7) / (pow(barC, 7) + pow25To7)));
	/* Equation 5 */
	double a1Prime = (1.0 + G) * lab1.a;
	double a2Prime = (1.0 + G) * lab2.a;
	/* Equation 6 */
	double CPrime1 = sqrt((a1Prime * a1Prime) + (lab1.b * lab1.b));
	double CPrime2 = sqrt((a2Prime * a2Prime) + (lab2.b * lab2.b));

	/* Equation 7 */
	double hPrime1;
	if (lab1.b == 0 && a1Prime == 0)
		hPrime1 = 0.0;
	else {
		hPrime1 = atan2(lab1.b, a1Prime);
		/*
		 * This must be converted to a hue angle in degrees between 0
		 * and 360 by addition of 2􏰏 to negative hue angles.
		 */
		if (hPrime1 < 0)
			hPrime1 += deg360InRad;
	}
	double hPrime2;
	if (lab2.b == 0 && a2Prime == 0)
		hPrime2 = 0.0;
	else {
		hPrime2 = atan2(lab2.b, a2Prime);
		/*
		 * This must be converted to a hue angle in degrees between 0
		 * and 360 by addition of 2􏰏 to negative hue angles.
		 */
		if (hPrime2 < 0)
			hPrime2 += deg360InRad;
	}

	/*
	 * Step 2
	 */
	/* Equation 8 */
	double deltaLPrime = lab2.l - lab1.l;
	/* Equation 9 */
	double deltaCPrime = CPrime2 - CPrime1;
	/* Equation 10 */
	double deltahPrime;
	double CPrimeProduct = CPrime1 * CPrime2;
	if (CPrimeProduct == 0)
		deltahPrime = 0;
	else {
		/* Avoid the fabs() call */
		deltahPrime = hPrime2 - hPrime1;
		if (deltahPrime < -deg180InRad)
			deltahPrime += deg360InRad;
		else if (deltahPrime > deg180InRad)
			deltahPrime -= deg360InRad;
	}
	/* Equation 11 */
	double deltaHPrime = 2.0 * sqrt(CPrimeProduct) * sin(deltahPrime / 2.0);

	/*
	 * Step 3
	 */
	/* Equation 12 */
	double barLPrime = (lab1.l + lab2.l) / 2.0;
	/* Equation 13 */
	double barCPrime = (CPrime1 + CPrime2) / 2.0;
	/* Equation 14 */
	double barhPrime, hPrimeSum = hPrime1 + hPrime2;
	if (CPrime1 * CPrime2 == 0) {
		barhPrime = hPrimeSum;
	} else {
		if (fabs(hPrime1 - hPrime2) <= deg180InRad)
			barhPrime = hPrimeSum / 2.0;
		else {
			if (hPrimeSum < deg360InRad)
				barhPrime = (hPrimeSum + deg360InRad) / 2.0;
			else
				barhPrime = (hPrimeSum - deg360InRad) / 2.0;
		}
	}

	/* Equation 15 */
	double T = 1.0 - (0.17 * cos(barhPrime - CIEDE2000::deg2Rad(30.0))) +
		(0.24 * cos(2.0 * barhPrime)) +
		(0.32 * cos((3.0 * barhPrime) + CIEDE2000::deg2Rad(6.0))) -
		(0.20 * cos((4.0 * barhPrime) - CIEDE2000::deg2Rad(63.0)));
	/* Equation 16 */
	double deltaTheta = CIEDE2000::deg2Rad(30.0) *
		exp(-pow((barhPrime - deg2Rad(275.0)) / deg2Rad(25.0), 2.0));
	/* Equation 17 */
	double R_C = 2.0 * sqrt(pow(barCPrime, 7.0) / (pow(barCPrime, 7.0) + pow25To7));
	/* Equation 18 */
	double S_L = 1 + ((0.015 * pow(barLPrime - 50.0, 2.0)) /
		sqrt(20 + pow(barLPrime - 50.0, 2.0)));
	/* Equation 19 */
	double S_C = 1 + (0.045 * barCPrime);
	/* Equation 20 */
	double S_H = 1 + (0.015 * barCPrime * T);
	/* Equation 21 */
	double R_T = (-sin(2.0 * deltaTheta)) * R_C;

	/* Equation 22 */
	double deltaE = sqrt(
		pow(deltaLPrime / (k_L * S_L), 2.0) +
		pow(deltaCPrime / (k_C * S_C), 2.0) +
		pow(deltaHPrime / (k_H * S_H), 2.0) +
		(R_T * (deltaCPrime / (k_C * S_C)) * (deltaHPrime / (k_H * S_H))));

	return (deltaE);
}

/*****************************************************************************
 * RGB to LAB conversion functions for ESP32 color matching
 *****************************************************************************/

void rgbToXYZ(uint8_t r, uint8_t g, uint8_t b, double &x, double &y, double &z) {
    // Normalize RGB values to 0-1 range
    double rNorm = r / 255.0;
    double gNorm = g / 255.0;
    double bNorm = b / 255.0;
    
    // Apply gamma correction (sRGB)
    auto gammaCorrect = [](double c) {
        if (c > 0.04045) {
            return pow((c + 0.055) / 1.055, 2.4);
        } else {
            return c / 12.92;
        }
    };
    
    rNorm = gammaCorrect(rNorm);
    gNorm = gammaCorrect(gNorm);
    bNorm = gammaCorrect(bNorm);
    
    // Convert to XYZ using sRGB matrix (D65 illuminant)
    x = rNorm * 0.4124564 + gNorm * 0.3575761 + bNorm * 0.1804375;
    y = rNorm * 0.2126729 + gNorm * 0.7151522 + bNorm * 0.0721750;
    z = rNorm * 0.0193339 + gNorm * 0.1191920 + bNorm * 0.9503041;
    
    // Scale by D65 illuminant
    x *= 100.0;
    y *= 100.0;
    z *= 100.0;
}

void xyzToLAB(double x, double y, double z, CIEDE2000::LAB &lab) {
    // D65 illuminant reference values
    const double Xn = 95.047;
    const double Yn = 100.000;
    const double Zn = 108.883;
    
    // Normalize by illuminant
    double xr = x / Xn;
    double yr = y / Yn;
    double zr = z / Zn;
    
    // Apply LAB transformation function
    auto labTransform = [](double t) {
        if (t > 0.008856) {
            return pow(t, 1.0/3.0);
        } else {
            return (7.787 * t) + (16.0/116.0);
        }
    };
    
    double fx = labTransform(xr);
    double fy = labTransform(yr);
    double fz = labTransform(zr);
    
    // Calculate LAB values
    lab.l = (116.0 * fy) - 16.0;
    lab.a = 500.0 * (fx - fy);
    lab.b = 200.0 * (fy - fz);
}

void rgbToLAB(uint8_t r, uint8_t g, uint8_t b, CIEDE2000::LAB &lab) {
    double x, y, z;
    rgbToXYZ(r, g, b, x, y, z);
    xyzToLAB(x, y, z, lab);
}

/*****************************************************************************
 * Operators.
 *****************************************************************************/

std::ostream& operator<<(
	std::ostream &s,
	const CIEDE2000::LAB &labColor)
{
	return (s << "CIELAB(" << labColor.l << "," << labColor.a << "," << labColor.b << ")");
}
