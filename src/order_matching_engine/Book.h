#ifndef BOOK_H
#define BOOK_H

#pragma once

#include "Logger.h"
#include "IOrderBook.h"
#include "strategy/IOrderMatchingStrategy.h"
#include <map>
#include <deque>
#include <mutex>
#include <queue>
#include <memory>
#include <vector>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <nlohmann/json.hpp>

using std::map;
using std::deque;
using std::vector;
using std::shared_ptr;
using std::unique_ptr;
using std::ostringstream;
using std::unordered_map;
using std::priority_queue;
using std::invalid_argument;

using json = nlohmann::json;

enum BOOK_STRATEGY { PRICE_TIME_PRIORITY };

class Book : public IOrderBook {
private:
	std::mutex bookMutex;
	unordered_map<string, shared_ptr<Order>> allOrders;
	map<double, shared_ptr<BuyBucket>> buyOrders;
	map<double, shared_ptr<SellBucket>> sellOrders;
	shared_ptr<IOrderMatchingStrategy> matcher;
	BOOK_STRATEGY strategy;
	shared_ptr<Security> security;
	void insertOrderUnlocked(std::shared_ptr<Order> order);
	void cancelOrderUnlocked(const std::string& orderId);
	shared_ptr<Order> orderLookupUnlocked(const std::string& orderId);

public:
	explicit Book(shared_ptr<IOrderMatchingStrategy> matcher, shared_ptr<Security> security);

	void insertOrder(shared_ptr<Order> order);
	void cancelOrder(const string& orderId);
	void modifyOrder(const string& orderId, double newQty, double newPrice);
	void setMatchingStrategy(std::shared_ptr<IOrderMatchingStrategy> newMatcher);
	void setSecurity(std::shared_ptr<Security> security);
	shared_ptr<Order> orderLookup(const string& orderId); // includes lock, when called by itself

	shared_ptr<Security> getSecurity() override;
	map<double, shared_ptr<BuyBucket>>& getBuyOrders() override;
	map<double, shared_ptr<SellBucket>>& getSellOrders() override;

	void cleanUpBuckets(shared_ptr<Order> order) override;
	void printBuyOrders() override;
	void printBuyOrdersFromAll() override;
	void printSellOrders() override;
	void printSellOrdersFromAll() override;

	void exportSnapshot() const;
	json toJson() const;

};

#endif // BOOK_H