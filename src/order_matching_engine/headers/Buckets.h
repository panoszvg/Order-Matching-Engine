#ifndef BUCKETS_H
#define BUCKETS_H

#pragma once

#include "core/Order.h"
#include <set>
#include <string>
#include <unordered_map>

using OrderMap = std::unordered_map<std::string, Order>;

struct BuyEntry {
	double   price;
	int64_t  timestamp;
	std::string id;

	bool operator<(const BuyEntry& o) const {
		if (price     != o.price)     return price > o.price;         // higher price first
		if (timestamp != o.timestamp) return timestamp < o.timestamp; // earlier first
		return id < o.id;
	}
};

struct SellEntry {
	double   price;
	int64_t  timestamp;
	std::string id;

	bool operator<(const SellEntry& o) const {
		if (price     != o.price)     return price < o.price;         // lower price first
		if (timestamp != o.timestamp) return timestamp < o.timestamp; // earlier first
		return id < o.id;
	}
};

template<typename Entry>
struct Bucket {
	std::set<Entry> queue;
	std::unordered_map<std::string, typename std::set<Entry>::iterator> index;
	double total_quantity = 0;

	void push(const Entry& e) {
		auto [it, ok] = queue.insert(e);
		if (ok) index.emplace(e.id, it);
	}

	void erase(const std::string& id) {
		auto it = index.find(id);
		if (it != index.end()) {
			queue.erase(it->second);
			index.erase(it);
		}
	}

	void popTop() {
		auto top = queue.begin();
		index.erase(top->id);
		queue.erase(top);
	}

	bool empty() const { return queue.empty(); }
	const Entry& top() const { return *queue.begin(); }
};

using BuyBucket  = Bucket<BuyEntry>;
using SellBucket = Bucket<SellEntry>;

#endif // BUCKETS_H
