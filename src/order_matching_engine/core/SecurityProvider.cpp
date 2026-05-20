#include "Book.h"
#include "SecurityProvider.h"
#include "strategy/PriceTimePriorityStrategy.h"

#include <string>
#include <fstream>
#include <sstream>
#include <stdexcept>

std::unordered_map<string, std::unique_ptr<Security>> SecurityProvider::loadSecurities() {
	std::unordered_map<string, std::unique_ptr<Security>> securities;

	std::ifstream in(path);
	if (!in)
		throw std::runtime_error("Cannot open " + path);

	std::string line;

	while (std::getline(in, line)) {
		std::istringstream ss(line);
		std::string symbol, minQty, lotSize;

		std::getline(ss, symbol, ',');
		std::getline(ss, minQty, ',');
		std::getline(ss, lotSize, ',');

		securities[symbol] = std::make_unique<Security>(symbol, stod(minQty), stod(lotSize));
	}

	return securities;
}
