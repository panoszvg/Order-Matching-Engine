#ifndef IORDERBOOK_H
#define IORDERBOOK_H

#include "Order.h"
#include "Security.h"
#include "headers/Math.h"
#include "headers/Buckets.h"
#include <memory>
#include <unordered_map>

using std::unordered_map;

class IOrderBook {
public:
	virtual void insertOrder(Order& order) = 0;
	virtual void cancelOrder(const std::string& id) = 0;
	virtual void modifyOrder(const string& orderId, double newQty, double newPrice) = 0;
	virtual Order& orderLookup(const std::string& id) = 0;

	virtual Security& getSecurity() = 0;
	virtual OrderMap& getOrders() = 0;
	virtual BuyBucket& getBuyOrders() = 0;
	virtual SellBucket& getSellOrders() = 0;

	virtual void printBuyOrders() {
		// Default : do nothing
	}
	virtual void printBuyOrdersFromAll() {
		// Default : do nothing
	}
	virtual void printSellOrders() {
		// Default : do nothing
	}
	virtual void printSellOrdersFromAll() {
		// Default : do nothing
	}
	virtual ~IOrderBook() = default;
};

#endif // IORDERBOOK_H