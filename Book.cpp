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
	map<string, Security*> securities;

	const double EPSILON = 1e-7;

	double getPriceBucket(double price, double tick) {
		return floor(price / tick) * tick;
	}

	inline int compareDoubles(double a, double b) {
		if (fabs(a - b) < EPSILON) return 0;
		return (a < b) ? -1 : 1;
	}

public:
	map<string, shared_ptr<Order>> allOrders;
	Book() {}

	void addSecurities(vector<Security *>& securities) {
		for (auto& security : securities) {
			this->securities[security->getName()] = security;
		}
	}

	void printSellOrders() {
		if (sellOrders.empty()) {
			cout <<"\n# No Sell Orders\n";
			return;
		}
		cout <<"\n# Sell Orders\n=============\n";
		for (auto [securityString, bucketMap] : sellOrders) {
			for (auto [price, bucketContainer] : bucketMap) {
				cout <<"\nBucket for " <<price <<":\n  total_quantity: " <<bucketContainer->total_quantity <<endl;
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
			cout <<"\n# No Sell Orders\n";
			return;
		}
		cout <<"\n# Sell Orders\n=============\n";
		for (auto& [securityString, order] : allOrders) {
			if (order->type == order->SELL && (order->fulfilled == order->NOT_FULFILLED || order->fulfilled == order->PARTIALLY_FULFILLED)) {
				order->print();
			}
		}
	}

	void printBuyOrders() {
		if (buyOrders.empty()) {
			cout <<"\n# No Buy Orders\n";
			return;
		}
		cout <<"\n# Buy Orders\n=============\n";
		for (auto [securityString, bucketMap] : buyOrders) {
			for (auto [price, bucketContainer] : bucketMap) {
				cout <<"\nBucket for " <<price <<":\n  total_quantity: " <<bucketContainer->total_quantity <<endl;
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
			cout <<"\n# No Buy Orders\n";
			return;
		}
		cout <<"\n# Buy Orders\n=============\n";
		for (auto& [securityString, order] : allOrders) {
			if (order->type == order->BUY && (order->fulfilled == order->NOT_FULFILLED || order->fulfilled == order->PARTIALLY_FULFILLED)) {
				order->print();
			}
		}
	}

	void matchBuyOrder(shared_ptr<Order> newOrder) {
		map< double, shared_ptr<SellBucket> >::iterator maxIt;
		map< double, shared_ptr<SellBucket> >::iterator it;
		it = sellOrders[newOrder->security].begin();
		maxIt = it;
		double orderPriceBucket = getPriceBucket(newOrder->price, this->securities[newOrder->security]->getTickSize());
		while (it != sellOrders[newOrder->security].end() && it->first <= orderPriceBucket) {
			maxIt = it;
			it++;
		}
		if (maxIt == sellOrders[newOrder->security].end() || sellOrders.empty() || it == maxIt) {
			return;
		}
		it = sellOrders[newOrder->security].begin();

		maxIt++;
		// cout <<"Trying to match buy order with price: " <<newOrder->price <<", quantity: " <<newOrder.quantity <<" and maxIt: " <<maxIt->first <<endl;

		while (it != sellOrders[newOrder->security].end()
			  && newOrder->fulfilled != newOrder->FULLY_FULFILLED
			  && newOrder->fulfilled != newOrder->CANCELLED
			  && it != maxIt)
		{
			if (it->second->bucket.empty()) {
				it++;
				continue;
			}

			shared_ptr<SellBucket> bucketContainer = it->second;
			while (!bucketContainer->bucket.empty()
				  && newOrder->fulfilled != newOrder->FULLY_FULFILLED
				  && newOrder->fulfilled != newOrder->CANCELLED)
			{
				auto order = bucketContainer->bucket.top();
				if (compareDoubles(newOrder->price, order->price) < 0)
					break;
				double sellQuantity = order->quantity;
				if (compareDoubles(newOrder->quantity, sellQuantity) < 0) {
					// sell order is partially fulfilled
					order->quantity -= newOrder->quantity;
					order->fulfilled = order->PARTIALLY_FULFILLED;
					// buy order is fully fulfilled
					newOrder->quantity = 0;
					newOrder->fulfilled = newOrder->FULLY_FULFILLED;
				}
				else {
					// buy order is partially fulfilled
					newOrder->quantity -= sellQuantity;
					newOrder->fulfilled = newOrder->PARTIALLY_FULFILLED;
					// sell order is fully fulfilled and removed
					order->quantity = 0;
					order->fulfilled = order->FULLY_FULFILLED;
				}
				if (compareDoubles(newOrder->quantity, 0.0) == 0) {
					newOrder->fulfilled = newOrder->FULLY_FULFILLED;
				}

				bucketContainer->total_quantity -= (sellQuantity - order->quantity);

				// if sell order is fulfilled, remove
				if (order->fulfilled == order->FULLY_FULFILLED) {
					bucketContainer->bucket.pop();
				}

				if (newOrder->fulfilled == newOrder->FULLY_FULFILLED || newOrder->fulfilled == newOrder->CANCELLED)
					break;

			}
			it++;
		}
	}

	void matchSellOrder(shared_ptr<Order> newOrder) {
		map< double, shared_ptr<BuyBucket> >::reverse_iterator minIt;
		map< double, shared_ptr<BuyBucket> >::reverse_iterator it;
		it = buyOrders[newOrder->security].rbegin();
		minIt = it;
		double orderPriceBucket = getPriceBucket(newOrder->price, this->securities[newOrder->security]->getTickSize());
		while (it != buyOrders[newOrder->security].rend() && it->first >= orderPriceBucket) {
			minIt = it;
			it++;
		}
		if (minIt == buyOrders[newOrder->security].rend() || buyOrders.empty() || it == minIt) {
			return;
		}
		it = buyOrders[newOrder->security].rbegin();

		minIt++;
		// cout <<"Trying to match sell order with price: " <<newOrder->price <<", quantity: " <<newOrder->quantity <<" and maxIt: " <<minIt->first <<endl;

		while (it != buyOrders[newOrder->security].rend()
			  && newOrder->fulfilled != newOrder->FULLY_FULFILLED
			  && newOrder->fulfilled != newOrder->CANCELLED
			  && it != minIt)
		{
			if (it->second->bucket.empty()) {
				it++;
				continue;
			}

			shared_ptr<BuyBucket> bucketContainer = it->second;

			while (!bucketContainer->bucket.empty()
				&& newOrder->fulfilled != newOrder->FULLY_FULFILLED
				&& newOrder->fulfilled != newOrder->CANCELLED)
			{
				auto order = bucketContainer->bucket.top();
				if (compareDoubles(newOrder->price, order->price) > 0)
					break;
				double buyQuantity = order->quantity;
				if (compareDoubles(newOrder->quantity, buyQuantity) < 0) {
					// sell order is partially fulfilled
					order->quantity -= newOrder->quantity;
					order->fulfilled = order->PARTIALLY_FULFILLED;
					// buy order is fully fulfilled
					newOrder->quantity = 0;
					newOrder->fulfilled = newOrder->FULLY_FULFILLED;
				}
				else {
					// buy order is partially fulfilled
					newOrder->quantity -= buyQuantity;
					newOrder->fulfilled = newOrder->PARTIALLY_FULFILLED;
					// sell order is fully fulfilled and removed
					order->quantity = 0;
					order->fulfilled = order->FULLY_FULFILLED;
				}
				if (compareDoubles(newOrder->quantity, 0.0) == 0) {
					newOrder->fulfilled = newOrder->FULLY_FULFILLED;
				}

				bucketContainer->total_quantity -= (buyQuantity - order->quantity);

				// if buy order is fulfilled, remove
				if (order->fulfilled == order->FULLY_FULFILLED) {
					bucketContainer->bucket.pop();
				}

				if (newOrder->fulfilled == order->FULLY_FULFILLED || newOrder->fulfilled == newOrder->CANCELLED)
					break;

			}
			it++;
		}
	}

	void matchOrder(shared_ptr<Order> order) {
		if (order->type == order->BUY)
			matchBuyOrder(order);
		else matchSellOrder(order);
	}

	void insertOrder(shared_ptr<Order> order) {
		if (this->securities.find(order->security) == this->securities.end())
			return;
		matchOrder(order);
		shared_ptr<Order> newOrder = order;
		if (order->fulfilled != order->FULLY_FULFILLED && order->fulfilled != order->CANCELLED) {
			double priceBucket = getPriceBucket(order->price, this->securities[order->security]->getTickSize());
			if (order->type == order->BUY) {
				if (buyOrders[order->security][priceBucket] == nullptr)
					buyOrders[order->security][priceBucket] = make_shared<BuyBucket>();
				buyOrders[order->security][priceBucket]->bucket.push(newOrder);
				buyOrders[order->security][priceBucket]->total_quantity += newOrder->quantity;
			}
			else {
				if (sellOrders[order->security][priceBucket] == nullptr)
					sellOrders[order->security][priceBucket] = make_shared<SellBucket>();
				sellOrders[order->security][priceBucket]->bucket.push(newOrder);
				sellOrders[order->security][priceBucket]->total_quantity += newOrder->quantity;
			}
		}
		allOrders.insert({newOrder->identifier, newOrder});
	}

};