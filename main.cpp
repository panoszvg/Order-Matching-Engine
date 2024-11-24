#include "Book.cpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
using namespace std;

int main() {

	Book *book = new Book();
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

		auto newOrder = new Order(security, (sell == "SELL") ? 1 : 0, stod(quantity), stod(price));
		book->insertOrder(*newOrder);
		// cout <<"After: " <<security <<" " <<sell <<" " <<price; newOrder->print(); cout <<endl <<endl <<endl;

	}


	cout <<"\n\nBook after ending:\n";
	book->printBuyOrders();
	book->printSellOrders();
	cout <<endl;

	return 0;
}