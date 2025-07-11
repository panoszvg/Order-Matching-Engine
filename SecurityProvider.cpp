#include "SecurityProvider.h"

#include <string>
#include <fstream>
#include <sstream>
#include <stdexcept>

std::vector<std::shared_ptr<Security>> SecurityProvider::loadSecurities() {
	std::vector<std::shared_ptr<Security>> securities;

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

		securities.push_back(std::make_shared<Security>(symbol, stod(minQty), stod(lotSize)));
	}

	return securities;
}
