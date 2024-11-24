#include <string>
#include <iostream>
#include <random>
#include <sstream>
using namespace std;

static random_device              rd;
static mt19937                    gen(rd());
static uniform_int_distribution<> dis(0, 15);
static uniform_int_distribution<> dis2(8, 11);

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


	string generateUUID() {
		stringstream ss;
		int i;
		ss << std::hex;
		for (i = 0; i < 8; i++) ss << dis(gen);
		ss << "-";
		for (i = 0; i < 4; i++) ss << dis(gen);
		ss << "-4";
		for (i = 0; i < 3; i++) ss << dis(gen);
		ss << "-";
		ss << dis2(gen);
		for (i = 0; i < 3; i++) ss << dis(gen);
		ss << "-";
		for (i = 0; i < 12; i++) ss << dis(gen);
		return ss.str();
	}

	Order(std::string security, bool sell, double quantity, double price = -1.0) {
		this->identifier = generateUUID();
		this->security = security;
		this->type = (sell) ? SELL : BUY;
		this->total_quantity = quantity;
		this->quantity = quantity;
		this->price = price;
		this->fulfilled = NOT_FULFILLED;
	}

	void print() {
		cout <<"  Order " <<this->identifier
		<<"\n\tQuantity: " <<this->quantity <<", Fulfilled: " <<((this->fulfilled == NOT_FULFILLED) ? "no" : (this->fulfilled == FULLY_FULFILLED) ? "yes" : ((this->fulfilled == PARTIALLY_FULFILLED) ? "partially" : "cancelled")) <<endl;
	}

};