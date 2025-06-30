#include "Security.h"

Security::Security(string symbol, double tickSize, double bucketSize)
    : symbol(std::move(symbol)), tickSize(tickSize), bucketSize(bucketSize) {}

const string& Security::getSymbol() const {
    return symbol;
}

double Security::getTickSize() const {
    return tickSize;
}

double Security::getBucketSize() const {
    return bucketSize;
}
