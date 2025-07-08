#ifndef MATH_H
#define MATH_H

#pragma once

#include <cmath>

constexpr double EPSILON = 1e-8;

inline double getPriceBucket(double price, double bucket_size) {
	return std::floor(price / bucket_size) * bucket_size;
}

inline int compareDoubles(double a, double b) {
	if (std::fabs(a - b) < EPSILON) return 0;
	return (a < b) ? -1 : 1;
}

inline bool isPriceTickAligned(double price, double tick) {
	double ratio = price / tick;
	double rounded = std::round(ratio);
	return std::fabs(ratio - rounded) < EPSILON;
}

#endif // MATH_H
