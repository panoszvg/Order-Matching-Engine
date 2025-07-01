#include "Book.h"
#include "Order.h"
#include "Parser.h"
#include "Security.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using std::ifstream;
using std::make_shared;
using std::stringstream;

/*	This is a function that creates securities. Ideally,
	it would not be used every time the application started,
	and if it did, it would load this information from a DB.
*/
void createSecurities(vector<shared_ptr<Security>>& securities){
	securities.push_back(make_shared<Security>("ADA", 0.001, 0.05));
	securities.push_back(make_shared<Security>("BTC", 0.01, 0.5));
}

int main() {

	logger->info("");
	logger->info("PROGRAM START");
	logger->info("");


	shared_ptr<IOrderBook> book = std::make_shared<Book>();
	vector<shared_ptr<Security>> securities;
	createSecurities(securities);
	book->addSecurities(securities);
	string line{""};
	vector<string> lines;
	ifstream file("FIX_Messages.csv");

	while (getline(file, line)) {
		lines.push_back(line);
	}
	file.close();

	shared_ptr<Parser> parser = make_shared<Parser>();

	for (auto& line : lines) {
		shared_ptr<Order> newOrder = parser->parse(line);
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