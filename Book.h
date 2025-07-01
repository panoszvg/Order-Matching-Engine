#ifndef BOOK_H
#define BOOK_H

#pragma once

#include "Math.h"
#include "Order.h"
#include "Buckets.h"
#include "Matcher.h"
#include "Security.h"
#include "IOrderBook.h"
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
using std::ostringstream;
using std::unordered_map;
using std::priority_queue;
using std::invalid_argument;

class Book : public IOrderBook {
	friend class Matcher;

private:
    unordered_map<string, map<double, shared_ptr<BuyBucket>>> buyOrders;
    unordered_map<string, map<double, shared_ptr<SellBucket>>> sellOrders;
    unordered_map<string, shared_ptr<Security>> securities;
    unordered_map<string, shared_ptr<Order>> allOrders;
	Matcher matcher;

public:
    Book();

    void addSecurities(vector<shared_ptr<Security>>& securities);
    void insertOrder(shared_ptr<Order> order);
	void cancelOrder(const string& orderId);
	void modifyOrder(const string& orderId, double newQty, double newPrice);
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