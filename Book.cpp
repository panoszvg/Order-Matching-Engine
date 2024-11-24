#include "Order.cpp"
#include <string>
#include <map>
#include <vector>
#include <deque>
#include <iostream>
using namespace std;

class Book {
private:
	const int PRECISION = 1, DECIMALS = 4;
	map< string, map< double, deque<Order> > > buyOrders;
	map< string, map< double, deque<Order> > > sellOrders;

public:
	map<string, Order> allOrders;
	Book() {}

	void inline cleanUpBook() {
		map< string, map< double, deque<Order> > >::iterator security;
		std::map<double, std::deque<Order>>::iterator row;
		security = buyOrders.begin();
		while (security != buyOrders.end()) {
			row = security->second.begin();
			while (row != security->second.end()) {
				if (row->second.empty()) {
					row = security->second.erase(row);
					continue;
				}
				else row++;
			}
			if (security->second.empty()) {
				security = buyOrders.erase(security);
				continue;
			}
			else security++;
		}
		security = sellOrders.begin();
		while (security != sellOrders.end()) {
			row = security->second.begin();
			while (row != security->second.end()) {
				if (row->second.empty()) {
					row = security->second.erase(row);
					continue;
				}
				else row++;
			}
			if (security->second.empty()) {
				security = sellOrders.erase(security);
				continue;
			}
			else security++;
		}
	}

	void printSellOrders() {
		if (sellOrders.empty()) {
			cout <<"\n# No Sell Orders\n";
			return;
		}
		cout <<"\n# Sell Orders\n=============\n";
		for (auto& securityPair : sellOrders) {
			for (auto& row : securityPair.second) {
				cout <<row.first <<": " <<endl;
				for (auto& order : row.second) {
					order.print();
				}
			}
		}
	}

	void printBuyOrders() {
		if (buyOrders.empty()) {
			cout <<"\n# No Buy Orders\n";
			return;
		}
		cout <<"\n# Buy Orders\n=============\n";
		for (auto& securityPair : buyOrders) {
			for (auto& row : securityPair.second) {
				cout <<row.first <<": " <<endl;
				for (auto& order : row.second) {
					order.print();
				}
			}
		}
	}

	void matchBuyOrder(Order &newOrder) {
		map<double, deque<Order>>::iterator maxIt;
		map<double, deque<Order>>::iterator it;
		it = sellOrders[newOrder.security].begin();
		maxIt = it;
		while (it != sellOrders[newOrder.security].end() && it->first <= newOrder.price) {
			maxIt = it;
			it++;
		}
		if (maxIt == sellOrders[newOrder.security].end() || sellOrders.empty() || sellOrders[newOrder.security].empty() ||  maxIt->second.empty() || it == maxIt) {
			return;
		}
		it = sellOrders[newOrder.security].begin();

		maxIt++;
		// cout <<"Trying to match buy order with price: " <<newOrder.price <<" and quantity: " <<newOrder.quantity <<endl;

		while (it != sellOrders[newOrder.security].end()
			  && newOrder.fulfilled != newOrder.FULLY_FULFILLED
			  && newOrder.fulfilled != newOrder.CANCELLED
			  && it != maxIt)
		{
			if (it->second.empty()) {
				it++;
				continue;
			}


			// get all orders with current value
			auto order = it->second.begin();

			while (order != it->second.end()) {
				// cout <<"Checking buy order with price: " <<order->price <<", quantity: " <<order->quantity <<endl;
				int buyQuantity = newOrder.quantity;
				int sellQuantity = order->quantity;
				if (buyQuantity < sellQuantity) {
					// sell order is partially fulfilled
					order->quantity -= buyQuantity;
					order->fulfilled = order->PARTIALLY_FULFILLED;
					// buy order is fully fulfilled
					newOrder.quantity = 0;
					newOrder.fulfilled = newOrder.FULLY_FULFILLED;
				}
				else {
					// buy order is partially fulfilled
					newOrder.quantity -= sellQuantity;
					newOrder.fulfilled = newOrder.PARTIALLY_FULFILLED;
					// sell order is fully fulfilled and removed
					order->quantity = 0;
					order->fulfilled = order->FULLY_FULFILLED;

					if (newOrder.quantity == 0) {
						newOrder.fulfilled = newOrder.FULLY_FULFILLED;
					}
				}
				allOrders.at(order->identifier).quantity = order->quantity;
				allOrders.at(order->identifier).fulfilled = order->fulfilled;
				if (order->fulfilled == order->FULLY_FULFILLED) {
					order = sellOrders[order->security].at(it->first).erase(order);
				}
				else {
					order++;
				}

				if (newOrder.fulfilled == 2 || newOrder.fulfilled == 3)
					break;

			}
			it++;
		}
		cleanUpBook();
	}

	void matchSellOrder(Order &newOrder) {
		map<double, deque<Order>>::reverse_iterator minIt;
		map<double, deque<Order>>::reverse_iterator it;
		it = buyOrders[newOrder.security].rbegin();
		minIt = it;
		while (it != buyOrders[newOrder.security].rend() && it->first >= newOrder.price) {
			minIt = it;
			it++;
		}
		if (minIt == buyOrders[newOrder.security].rend() || buyOrders.empty() || buyOrders[newOrder.security].empty() ||  minIt->second.empty() || it == minIt) {
			return;
		}
		it = buyOrders[newOrder.security].rbegin();

		minIt++;
		// cout <<"Trying to match sell order with price: " <<newOrder.price <<" and quantity: " <<newOrder.quantity <<endl;

		while (it != buyOrders[newOrder.security].rend()
			  && newOrder.fulfilled != newOrder.FULLY_FULFILLED
			  && newOrder.fulfilled != newOrder.CANCELLED
			  && it != minIt)
		{
			if (it->second.empty()) {
				it++;
				continue;
			}

			// get all orders with current value
			auto order = it->second.begin();

			while (order != it->second.end()) {
				// cout <<"Checking sell order with price: " <<order->price <<", quantity: " <<order->quantity <<endl;
				int buyQuantity = newOrder.quantity;
				int sellQuantity = order->quantity;
				if (buyQuantity < sellQuantity) {
					// sell order is partially fulfilled
					order->quantity -= buyQuantity;
					order->fulfilled = order->PARTIALLY_FULFILLED;
					// buy order is fully fulfilled
					newOrder.quantity = 0.0;
					newOrder.fulfilled = newOrder.FULLY_FULFILLED;
				}
				else {
					// buy order is partially fulfilled
					newOrder.quantity -= sellQuantity;
					newOrder.fulfilled = newOrder.PARTIALLY_FULFILLED;
					// sell order is fully fulfilled and removed
					order->quantity = 0.0;
					order->fulfilled = order->FULLY_FULFILLED;

					if (newOrder.quantity == 0.0) {
						newOrder.fulfilled = newOrder.FULLY_FULFILLED;
					}
				}
				allOrders.at(order->identifier).quantity = order->quantity;
				allOrders.at(order->identifier).fulfilled = order->fulfilled;
				if (order->fulfilled == order->FULLY_FULFILLED) {
					order = buyOrders[order->security].at(it->first).erase(order);
				}
				else {
					order++;
				}

				if (newOrder.fulfilled == 2 || newOrder.fulfilled == 3)
					break;

			}
			it++;
		}
		cleanUpBook();
	}

	void matchOrder(Order& order) {
		if (order.type == order.BUY)
			matchBuyOrder(order);
		else matchSellOrder(order);
	}

	void insertOrder(Order order) {
		matchOrder(order);
		if (order.fulfilled != 2 && order.fulfilled != 3) {
			if (order.type == order.BUY) {
				buyOrders[order.security][order.price].push_back(order);
			}
			else {
				sellOrders[order.security][order.price].push_back(order);
			}
		}
		allOrders.insert({order.identifier, order});
	}

};