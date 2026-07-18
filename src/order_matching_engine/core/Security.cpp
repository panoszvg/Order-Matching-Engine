#include "Security.h"
#include <sstream>
#include <stdexcept>

Security::Security(string symbol, double tickSize, double bucketSize)
	: symbol(std::move(symbol)), tickSize(tickSize), bucketSize(bucketSize) {
	if (tickSize <= 0.0) {
		std::ostringstream oss;
		oss << "Tick size (" << tickSize << ") must be greater than zero";
		throw std::invalid_argument(oss.str());
	}

	if (bucketSize <= 0.0) {
		std::ostringstream oss;
		oss << "Bucket size (" << bucketSize << ") must be greater than zero";
		throw std::invalid_argument(oss.str());
	}
}

const string& Security::getSymbol() const {
	return symbol;
}

double Security::getTickSize() const {
	return tickSize;
}

double Security::getBucketSize() const {
	return bucketSize;
}
