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

	std::unordered_map<string, shared_ptr<Book>> books;
	
	try {
		SecurityProvider provider("input_files/securities.csv");
		std::unordered_map<string, shared_ptr<Security>> securities = provider.loadSecurities();
		auto strategy = std::make_shared<PriceTimePriorityStrategy>();
		for (auto [securityStr, security] : securities) {
			auto book = std::make_shared<Book>(strategy, security);
			books[securityStr] = book;
		}
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
			books[newOrder->security]->insertOrder(newOrder);
		} catch (const std::invalid_argument& arg) {
			logger->error("Order rejected: {}", arg.what());
		}
	}

	for (auto& [security, book] : books) {
		logger->info("{} Book after ending:", security);
		book->printBuyOrders();
		book->printSellOrders();
	}

	// for (auto& order : book->allOrders) {
	// 	order.second->print();
	// }

	return 0;
}