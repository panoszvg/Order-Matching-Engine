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
	std::map<double, std::deque<Order>> buyOrders;
	std::map<double, std::deque<Order>> sellOrders;

public:
	Book() {}

	void inline cleanUpBook() {
		auto row = buyOrders.begin();
		while (row != buyOrders.end()) {
			if (row->second.empty()) {
				row = buyOrders.erase(row);
				continue;
			}
			else row++;
		}
		row = sellOrders.begin();
		while (row != sellOrders.end()) {
			if (row->second.empty()) {
				row = sellOrders.erase(row);
				continue;
			}
			else row++;
		}
	}

	void printSellOrders() {
		if (sellOrders.empty()) {
			cout <<"\n# No Sell Orders\n";
			return;
		}
		cout <<"\n# Sell Orders\n=============\n";
		for (auto& row : sellOrders) {
			cout <<row.first <<": " <<endl;
			for (auto& order : row.second) {
				order.print();
			}
		}
	}

	void printBuyOrders() {
		if (buyOrders.empty()) {
			cout <<"\n# No Buy Orders\n";
			return;
		}
		cout <<"\n# Buy Orders\n=============\n";
		for (auto& row : buyOrders) {
			cout <<row.first <<": " <<endl;
			for (auto& order : row.second) {
				order.print();
			}
		}
	}

	void matchBuyOrder(Order &newOrder) {
		std::map<double, std::deque<Order>>::iterator maxIt;
		std::map<double, std::deque<Order>>::iterator it;
		it = sellOrders.begin();
		maxIt = it;
		while (it != sellOrders.end() && it->first <= newOrder.price) {
			maxIt = it;
			it++;
		}
		if (maxIt == sellOrders.end() || sellOrders.empty() ||  maxIt->second.empty() || it == maxIt) {
			return;
		}
		it = sellOrders.begin();

		maxIt++;
		// cout <<"Trying to match order with price: " <<newOrder.price <<" and quantity: " <<newOrder.quantity <<endl;

		while (it != sellOrders.end()
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
				// cout <<"Checking sell order with price: " <<order->price <<", quantity: " <<order->quantity <<endl;
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

				if (order->fulfilled == order->FULLY_FULFILLED) {
					sellOrders.at(it->first).erase(order);
				}
				else {
					order++;
				}
			}
			it++;
		}
		cleanUpBook();
	}

	void matchSellOrder(Order &newOrder) {
		std::map<double, std::deque<Order>>::reverse_iterator minIt;
		std::map<double, std::deque<Order>>::reverse_iterator it;
		it = buyOrders.rbegin();
		minIt = it;
		while (it != buyOrders.rend() && it->first >= newOrder.price) {
			minIt = it;
			it++;
		}
		if (minIt == buyOrders.rend() || buyOrders.empty() ||  minIt->second.empty() || it == minIt) {
			return;
		}
		it = buyOrders.rbegin();

		minIt++;
		// cout <<"Trying to match order with price: " <<newOrder.price <<" and quantity: " <<newOrder.quantity <<endl;

		while (it != buyOrders.rend()
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
				order->print();
				if (order->fulfilled == order->FULLY_FULFILLED) {
					buyOrders.at(it->first).erase(order);
				}
				else {
					order++;
				}

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
				buyOrders[order.price].push_back(order);
			}
			else {
				sellOrders[order.price].push_back(order);
			}
		}
		printBuyOrders();
		printSellOrders();
		cout <<endl;
	}

};