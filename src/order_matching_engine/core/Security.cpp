#include "Security.h"
#include <sstream>
#include <stdexcept>

Security::Security(string symbol, double tickSize, double lotSize)
	: symbol(std::move(symbol)), tickSize(tickSize), lotSize(lotSize) {
	if (tickSize <= 0.0) {
		std::ostringstream oss;
		oss << "Tick size (" << tickSize << ") must be greater than zero";
		throw std::invalid_argument(oss.str());
	}

	if (lotSize <= 0.0) {
		std::ostringstream oss;
		oss << "Lot size (" << lotSize << ") must be greater than zero";
		throw std::invalid_argument(oss.str());
	}
}

const string& Security::getSymbol() const {
	return symbol;
}

double Security::getTickSize() const {
	return tickSize;
}

double Security::getLotSize() const {
	return lotSize;
}
