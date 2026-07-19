#ifndef BOOK_H
#define BOOK_H

#pragma once

#include "Logger.h"
#include "IOrderBook.h"
#include "strategy/IOrderMatchingStrategy.h"
#include <mutex>
#include <memory>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <nlohmann/json.hpp>

using std::unique_ptr;
using std::ostringstream;
using std::unordered_map;
using std::invalid_argument;

using json = nlohmann::json;

class Book : public IOrderBook {
private:
	mutable std::mutex bookMutex;
	unordered_map<string, Order> allOrders;
	BuyBucket  buyOrders;
	SellBucket sellOrders;
	unique_ptr<IOrderMatchingStrategy> matcher;
	unique_ptr<Security> security;
	void insertOrderUnlocked(Order& order);
	void cancelOrderUnlocked(const std::string& orderId);
	Order& orderLookupUnlocked(const std::string& orderId);
	json toJsonUnlocked() const;

public:
	explicit Book(unique_ptr<IOrderMatchingStrategy> matcher, unique_ptr<Security> security);

	void insertOrder(Order& order);
	void cancelOrder(const string& orderId);
	void modifyOrder(const string& orderId, double newQty, double newPrice);
	void setMatchingStrategy(std::unique_ptr<IOrderMatchingStrategy> newMatcher);
	void setSecurity(std::unique_ptr<Security> security);
	Order& orderLookup(const string& orderId) override;

	Security& getSecurity() override;
	OrderMap& getOrders() override;
	BuyBucket& getBuyOrders() override;
	SellBucket& getSellOrders() override;

	void printBuyOrders() override;
	void printBuyOrdersFromAll() override;
	void printSellOrders() override;
	void printSellOrdersFromAll() override;

	void exportSnapshot() const;
	json toJson() const;

};

#endif // BOOK_H