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
void createSecurities(vector<Security*>& securities){
	securities.push_back(new Security("ADA", 0.001));
	securities.push_back(new Security("BTC", 0.1));
}

int main() {

	Book *book = new Book();
	vector<Security*> securities;
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
		book->insertOrder(newOrder);
	}


	cout <<"\nBook after ending:\n";
	book->printBuyOrders();
	book->printSellOrders();
	cout <<endl;

	// int counter = 0;
	// for (auto& order : book->allOrders) {
	// 	cout <<++counter <<" ";
	// 	order.second->print(1);
	// 	cout <<endl;
	// }

	return 0;
}