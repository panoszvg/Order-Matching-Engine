#ifndef BOOK_H
#define BOOK_H

#include "Order.h"
#include "Security.h"
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
using std::unordered_map;
using std::priority_queue;

constexpr double EPSILON = 1e-8;

class Book {
private:
	class CompareBuy {
	public:
		bool operator() (shared_ptr<Order> o1, shared_ptr<Order> o2) {
			return o1->price < o2->price;
		}
	};
	class CompareSell {
	public:
		bool operator() (shared_ptr<Order> o1, shared_ptr<Order> o2) {
			return o1->price > o2->price;
		}
	};

	class BuyBucket {
	public:
		priority_queue<shared_ptr<Order>, deque<shared_ptr<Order>>, CompareBuy> bucket;
		double total_quantity = 0;
	};

	class SellBucket {
	public:
		priority_queue<shared_ptr<Order>, deque<shared_ptr<Order>>, CompareSell> bucket;
		double total_quantity = 0;
	};

    unordered_map<string, map<double, shared_ptr<BuyBucket>>> buyOrders;
    unordered_map<string, map<double, shared_ptr<SellBucket>>> sellOrders;
    unordered_map<string, shared_ptr<Security>> securities;

    unordered_map<string, shared_ptr<Order>> allOrders;

    double getPriceBucket(double price, double bucketSize);
    int compareDoubles(double a, double b);
    bool isPriceTickAligned(double price, double tick);

public:
    Book();

    void addSecurities(vector<shared_ptr<Security>>& securities);
    void insertOrder(shared_ptr<Order> order);
    void matchOrder(shared_ptr<Order> order);
    void matchBuyOrder(shared_ptr<Order> order);
    void matchSellOrder(shared_ptr<Order> order);
    void matchBuyAgainstBucket(shared_ptr<Order>& buyOrder, shared_ptr<SellBucket>& bucket);
    void matchSellAgainstBucket(shared_ptr<Order>& sellOrder, shared_ptr<BuyBucket>& bucket);

    void cleanUpBuckets(shared_ptr<Order> order);
    void printBuyOrders() const;
    void printBuyOrdersFromAll() const;
    void printSellOrders() const;
    void printSellOrdersFromAll() const;
    void printOrderBook() const;

};

#endif // BOOK_H