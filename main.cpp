#include "Book.cpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
using namespace std;

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


	unique_ptr<Book> book = make_unique<Book>();
	vector<shared_ptr<Security>> securities;
	createSecurities(securities);
	book->addSecurities(securities);
	string line{""};
	vector<string> lines;
	ifstream file("input.txt");

	while (getline(file, line)) {
		lines.push_back(line);
	}
	file.close();

	for (auto& line : lines) {
		string security, sell, quantity, price;
		stringstream ss(line);

		getline(ss,security, ',');
		getline(ss,sell, ',');
		getline(ss,quantity, ',');
		getline(ss,price, ',');

		shared_ptr<Order> newOrder = make_shared<Order>(security, (sell == "SELL") ? 1 : 0, stod(quantity), stod(price));
		try {
			book->insertOrder(newOrder);
		} catch (const invalid_argument& arg) {
			cerr << "Order rejected: " << arg.what() << endl;
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