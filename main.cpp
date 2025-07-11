#include "Book.h"
#include "Order.h"
#include "Logger.h"
#include "Parser.h"
#include "Security.h"
#include "SecurityProvider.h"
#include "strategy/PriceTimePriorityStrategy.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

int main() {

	logger->info("");
	logger->info("PROGRAM START");
	logger->info("");

	auto strategy = std::make_unique<PriceTimePriorityStrategy>();
	auto book = std::make_shared<Book>(std::move(strategy));
	book->setMatchingStrategy(std::make_unique<PriceTimePriorityStrategy>());

	try {
		SecurityProvider provider("input_files/securities.csv");
		vector<shared_ptr<Security>> securities = provider.loadSecurities();
		book->addSecurities(securities);
	} catch(std::exception &e) {
		logger->error("Securities couldn't be read: {}", e.what());
	}

	string line = "";
	vector<string> lines;
	std::ifstream file("input_files/FIX_Messages.csv");

	while (getline(file, line)) {
		lines.push_back(line);
	}
	file.close();

	shared_ptr<Parser> parser = std::make_shared<Parser>();

	for (auto& line : lines) {
		auto newMessage = parser->parse(line);
		auto newOrder = newMessage->makeOrder();
		try {
			book->insertOrder(newOrder);
		} catch (const std::invalid_argument& arg) {
			logger->error("Order rejected: {}", arg.what());
		}
	}

	logger->info("Book after ending:");
	book->printBuyOrders();
	book->printSellOrders();

	// for (auto& order : book->allOrders) {
	// 	order.second->print();
	// }

	return 0;
}