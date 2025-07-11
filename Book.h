#ifndef BOOK_H
#define BOOK_H

#pragma once

#include "Logger.h"
#include "IOrderBook.h"
#include "strategy/IOrderMatchingStrategy.h"
#include <map>
#include <deque>
#include <queue>
#include <memory>
#include <vector>
#include <sstream>
#include <unordered_map>

using std::map;
using std::deque;
using std::vector;
using std::shared_ptr;
using std::unique_ptr;
using std::ostringstream;
using std::unordered_map;
using std::priority_queue;
using std::invalid_argument;

enum BOOK_STRATEGY { PRICE_TIME_PRIORITY };

class Book : public IOrderBook {
private:
    unordered_map<string, map<double, shared_ptr<BuyBucket>>> buyOrders;
    unordered_map<string, map<double, shared_ptr<SellBucket>>> sellOrders;
    unordered_map<string, shared_ptr<Security>> securities;
    unordered_map<string, shared_ptr<Order>> allOrders;
	unique_ptr<IOrderMatchingStrategy> matcher;
    BOOK_STRATEGY strategy;

public:
    explicit Book(std::unique_ptr<IOrderMatchingStrategy> matcher);

    void addSecurities(vector<shared_ptr<Security>>& securities);
    void insertOrder(shared_ptr<Order> order);
	void cancelOrder(const string& orderId);
	void modifyOrder(const string& orderId, double newQty, double newPrice);
    void setMatchingStrategy(std::unique_ptr<IOrderMatchingStrategy> newMatcher);
	shared_ptr<Order> orderLookup(const string& orderId);

    unordered_map<string, shared_ptr<Security>>& getSecurities() override;
    unordered_map<string, map<double, shared_ptr<BuyBucket>>>& getBuyOrders() override;
    unordered_map<string, map<double, shared_ptr<SellBucket>>>& getSellOrders() override;

    void cleanUpBuckets(shared_ptr<Order> order) override;
    void printBuyOrders() override;
    void printBuyOrdersFromAll() override;
    void printSellOrders() override;
    void printSellOrdersFromAll() override;

};

#endif // BOOK_H