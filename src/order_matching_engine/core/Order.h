#ifndef ORDER_H
#define ORDER_H

#include <chrono>
#include <string>
#include <cstdint>

using std::string;

enum OrderType {
	BUY,
	SELL
};

enum Fulfilled {
	NOT_FULFILLED,
	PARTIALLY_FULFILLED,
	FULLY_FULFILLED,
	CANCELLED
};

class Order {
public:
	Order(string security, OrderType type, double quantity, double price);
	Order(string security, OrderType type, double quantity, double originalQuantity, double price);

	string id;
	string security;
	OrderType type;
	double originalQuantity;
	double quantity;
	double price;
	Fulfilled fulfilled;
	std::chrono::system_clock::time_point timestamp;

	int64_t getMicroTimestamp() const;
	void print() const;
};

string generateUUID();

#endif // ORDER_H
