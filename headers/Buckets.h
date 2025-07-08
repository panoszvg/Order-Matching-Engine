#ifndef BUCKETS_H
#define BUCKETS_H

#pragma once

#include "../Order.h"
#include <queue>
#include <deque>
#include <memory>

using std::deque;
using std::shared_ptr;
using std::priority_queue;

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

#endif // BUCKETS_H
