#include <string>
#include <iostream>
using namespace std;

class Order{
public:
	enum Type { BUY, SELL } type;

	std::string identifier;
	std::string security;
	double total_quantity;
	double quantity;
	double price;
	// 0: Not fulfilled, 1: Partially fulfilled, 2: Fully fulfilled, 3: Cancelled
	enum Fulfilled { NOT_FULFILLED, PARTIALLY_FULFILLED, FULLY_FULFILLED, CANCELLED } fulfilled;

	Order(std::string security, bool sell, double quantity, double price = -1.0) {
		this->security = security;
		this->type = (sell) ? SELL : BUY;
		this->total_quantity = quantity;
		this->quantity = quantity;
		this->price = price;
		this->fulfilled = NOT_FULFILLED;
	}

	void print() {
		cout <<"  Quantity: " <<this->quantity <<", Fulfilled: " <<((this->fulfilled == NOT_FULFILLED) ? "no" : "partially") <<endl;
	}

};