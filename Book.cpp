#include "Order.cpp"
#include "Security.cpp"
#include <string>
#include <map>
#include <vector>
#include <deque>
#include <queue>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <memory>
using namespace std;

class Book {
private:
	const int PRECISION = 1, DECIMALS = 4;
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

	map< string, map< double, shared_ptr<BuyBucket> > > buyOrders;
	map< string, map< double, shared_ptr<SellBucket> > > sellOrders;
	map<string, shared_ptr<Security>> securities;

	void cleanUpBuckets(shared_ptr<Order> order){
		if (order->type == Order::BUY) {
			auto& book = sellOrders[order->security];
			for (auto it = book.begin(); it != book.end(); ) {
				if (it->second->bucket.empty())
					it = book.erase(it);
				else
					++it;
				}
			}
		else {
			auto& book = buyOrders[order->security];
			for (auto it = book.begin(); it != book.end(); ) {
				if (it->second->bucket.empty())
					it = book.erase(it);
				else
					++it;
			}
		}
	}

	const double EPSILON = 1e-8;

	inline double getPriceBucket(double price, double bucket_size) {
		return floor(price / bucket_size) * bucket_size;
	}

	inline int compareDoubles(double a, double b) {
		if (fabs(a - b) < EPSILON) return 0;
		return (a < b) ? -1 : 1;
	}

	inline bool isPriceTickAligned(double price, double tick) {
		double ratio = price / tick;
		double rounded = round(ratio);
		return fabs(ratio - rounded) < EPSILON;
	}

public:
	map<string, shared_ptr<Order>> allOrders;
	Book() {}

	void addSecurities(vector<shared_ptr<Security>>& securities) {
		for (auto& security : securities) {
			this->securities[security->getName()] = security;
		}
	}

	void printSellOrders() {		
		if (sellOrders.empty()) {
			logger->info("No Sell Orders");
			return;
		}
		logger->info("# Sell Orders");
		logger->info("=============");
		for (auto [securityString, bucketMap] : sellOrders) {
			for (auto [price, bucketContainer] : bucketMap) {
				logger->info("Bucket for {:.6}:", price);
				logger->info("  total_quantity: {:.6}", bucketContainer->total_quantity);
				auto tempBucket = bucketContainer->bucket;
				while (!tempBucket.empty()) {
					auto top = tempBucket.top();
					tempBucket.pop();
					top->print();
				}
			}
		}
	}

	void printSellOrdersFromAll() {
		if (sellOrders.empty()) {
			logger->info("No Sell Orders");
			return;
		}
		logger->info("# Sell Orders");
		logger->info("=============");
		for (auto& [securityString, order] : allOrders) {
			if (order->type == order->SELL && (order->fulfilled == order->NOT_FULFILLED || order->fulfilled == order->PARTIALLY_FULFILLED)) {
				order->print();
			}
		}
	}

	void printBuyOrders() {
		if (buyOrders.empty()) {
			logger->info("# No Buy Orders");
			return;
		}
		logger->info("# Buy Orders");
		logger->info("=============");
		for (auto [securityString, bucketMap] : buyOrders) {
			for (auto [price, bucketContainer] : bucketMap) {
				logger->info("Bucket for {:.6}:", price);
				logger->info("  total_quantity: {:.6}", bucketContainer->total_quantity);
				auto tempBucket = bucketContainer->bucket;
				while (!tempBucket.empty()) {
					auto top = tempBucket.top();
					tempBucket.pop();
					top->print();
				}
			}
		}
	}

	void printBuyOrdersFromAll() {
		if (buyOrders.empty()) {
			logger->info("# No Buy Orders");
			return;
		}
		logger->info("# Buy Orders");
		logger->info("=============");
		for (auto& [securityString, order] : allOrders) {
			if (order->type == order->BUY && (order->fulfilled == order->NOT_FULFILLED || order->fulfilled == order->PARTIALLY_FULFILLED)) {
				order->print();
			}
		}
	}

	void matchBuyAgainstBucket(shared_ptr<Order>& buyOrder, shared_ptr<SellBucket>& bucket) {
		while (!bucket->bucket.empty()
			&& buyOrder->fulfilled != Order::FULLY_FULFILLED
			&& buyOrder->fulfilled != Order::CANCELLED)
		{
			auto sellOrder = bucket->bucket.top();

			// Stop if the sell price is too high
			if (compareDoubles(buyOrder->price, sellOrder->price) < 0)
				break;

			// Determine how much can be traded
			double tradedQty = min(buyOrder->quantity, sellOrder->quantity);

			// Subtract from both orders
			buyOrder->quantity -= tradedQty;
			sellOrder->quantity -= tradedQty;

			// Update fulfillment status for buy order
			if (compareDoubles(buyOrder->quantity, 0.0) == 0)
				buyOrder->fulfilled = Order::FULLY_FULFILLED;
			else
				buyOrder->fulfilled = Order::PARTIALLY_FULFILLED;

			// Update fulfillment status for sell order
			if (compareDoubles(sellOrder->quantity, 0.0) == 0) {
				sellOrder->fulfilled = Order::FULLY_FULFILLED;
				bucket->bucket.pop();
			} else {
				sellOrder->fulfilled = Order::PARTIALLY_FULFILLED;
			}

			// Reduce total quantity in the bucket
			bucket->total_quantity -= tradedQty;
		}
	}


	void matchBuyOrder(shared_ptr<Order> newOrder) {
		if (sellOrders.empty()) return;

		auto& sellBuckets = sellOrders[newOrder->security];
		if (sellBuckets.empty()) return;

		double orderPriceBucket = getPriceBucket(newOrder->price, securities[newOrder->security]->getBucketSize());

		auto it = sellBuckets.begin();
		auto endIt = it;
		while (endIt != sellBuckets.end() && endIt->first <= orderPriceBucket) {
			++endIt;
		}

		for (; it != endIt; ++it) {
			auto& bucket = it->second;
			if (bucket->bucket.empty()) continue;

			matchBuyAgainstBucket(newOrder, bucket);
		}
	}


	void matchSellAgainstBucket(shared_ptr<Order>& sellOrder, shared_ptr<BuyBucket>& bucket) {
		while (!bucket->bucket.empty()
			&& sellOrder->fulfilled != Order::FULLY_FULFILLED
			&& sellOrder->fulfilled != Order::CANCELLED)
		{
			auto buyOrder = bucket->bucket.top();

			// Stop if the buy price is too low
			if (compareDoubles(sellOrder->price, buyOrder->price) > 0)
				break;

			// Determine how much can be traded
			double tradedQty = min(sellOrder->quantity, buyOrder->quantity);

			// Subtract from both orders
			sellOrder->quantity -= tradedQty;
			buyOrder->quantity -= tradedQty;

			// Update fulfillment status for sell order
			if (compareDoubles(sellOrder->quantity, 0.0) == 0)
				sellOrder->fulfilled = Order::FULLY_FULFILLED;
			else
				sellOrder->fulfilled = Order::PARTIALLY_FULFILLED;

			// Update fulfillment status for buy order
			if (compareDoubles(buyOrder->quantity, 0.0) == 0) {
				buyOrder->fulfilled = Order::FULLY_FULFILLED;
				bucket->bucket.pop();
			} else {
				buyOrder->fulfilled = Order::PARTIALLY_FULFILLED;
			}

			// Reduce total quantity in the bucket
			bucket->total_quantity -= tradedQty;
		}
	}

	void matchSellOrder(shared_ptr<Order> newOrder) {
		if (buyOrders.empty()) return;

		auto& buyBuckets = buyOrders[newOrder->security];
		if (buyBuckets.empty()) return;

		double orderPriceBucket = getPriceBucket(newOrder->price, securities[newOrder->security]->getBucketSize());

		auto it = buyBuckets.rbegin();
		auto endIt = it;
		while (endIt != buyBuckets.rend() && endIt->first >= orderPriceBucket) {
			++endIt;
		}

		for (; it != endIt; ++it) {
			auto& bucket = it->second;
			if (bucket->bucket.empty()) continue;

			matchSellAgainstBucket(newOrder, bucket);
		}
	}


	void matchOrder(shared_ptr<Order> order) {
		if (order->type == order->BUY)
			matchBuyOrder(order);
		else matchSellOrder(order);
		cleanUpBuckets(order);
	}

	void insertOrder(shared_ptr<Order> order) {
		if (this->securities.find(order->security) == this->securities.end())
			return;

		auto orderSecurity = this->securities[order->security];
		double orderSecurityTick = orderSecurity->getTickSize();

		if (!isPriceTickAligned(order->price, orderSecurityTick)) {
			std::ostringstream oss;
			oss << "Order price (" << order->price << ") does not align with tick size (" << orderSecurityTick << ")";
			throw std::invalid_argument(oss.str());
		}

		matchOrder(order);
		auto newOrder = order;
		if (order->fulfilled != order->FULLY_FULFILLED && order->fulfilled != order->CANCELLED) {
			double priceBucket = getPriceBucket(order->price, this->securities[order->security]->getBucketSize());
			if (order->type == order->BUY) {
				if (buyOrders[order->security].find(priceBucket) == buyOrders[order->security].end())
					buyOrders[order->security][priceBucket] = make_shared<BuyBucket>();
				buyOrders[order->security][priceBucket]->bucket.push(newOrder);
				buyOrders[order->security][priceBucket]->total_quantity += newOrder->quantity;
			}
			else {
				if (sellOrders[order->security].find(priceBucket) == sellOrders[order->security].end())
					sellOrders[order->security][priceBucket] = make_shared<SellBucket>();
				sellOrders[order->security][priceBucket]->bucket.push(newOrder);
				sellOrders[order->security][priceBucket]->total_quantity += newOrder->quantity;
			}
		}
		allOrders.insert({newOrder->identifier, newOrder});
	}

};