#ifndef BUCKETS_H
#define BUCKETS_H

#pragma once

#include "core/Order.h"
#include <queue>
#include <string>
#include <vector>
#include <unordered_map>

using OrderMap = std::unordered_map<std::string, Order>;

struct CompareBuy {
	const OrderMap& orders;
	explicit CompareBuy(const OrderMap& orders) : orders(orders) {}
	bool operator()(const std::string& a, const std::string& b) const {
		const Order& oa = orders.at(a);
		const Order& ob = orders.at(b);
		if (oa.price == ob.price) return oa.timestamp > ob.timestamp;
		return oa.price < ob.price;
	}
};

struct CompareSell {
	const OrderMap& orders;
	explicit CompareSell(const OrderMap& orders) : orders(orders) {}
	bool operator()(const std::string& a, const std::string& b) const {
		const Order& oa = orders.at(a);
		const Order& ob = orders.at(b);
		if (oa.price == ob.price) return oa.timestamp > ob.timestamp;
		return oa.price > ob.price;
	}
};

template<typename Compare>
struct Bucket {
	std::priority_queue<std::string, std::vector<std::string>, Compare> queue;
	double total_quantity = 0;
	explicit Bucket(Compare cmp) : queue(std::move(cmp)) {}
};

using BuyBucket  = Bucket<CompareBuy>;
using SellBucket = Bucket<CompareSell>;

#endif // BUCKETS_H
