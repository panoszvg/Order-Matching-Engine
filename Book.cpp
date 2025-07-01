#include "Book.h"
#include <string>

Book::Book() : matcher(*this) {}

void Book::addSecurities(vector<shared_ptr<Security>>& securities) {
	for (auto& security : securities) {
		this->securities[security->getSymbol()] = security;
	}
}	

void Book::insertOrder(shared_ptr<Order> order) {
	if (this->securities.find(order->security) == this->securities.end())
	return;
	
	auto orderSecurity = this->securities[order->security];	
	double orderSecurityTick = orderSecurity->getTickSize();
	
	if (compareDoubles(order->price, 0.0) <= 0) {
		ostringstream oss;
		oss << "Price (" << order->price << ") must be greater than zero";
		throw invalid_argument(oss.str());
	}
	
	if (compareDoubles(order->quantity, 0.0) <= 0) {
		ostringstream oss;
		oss << "Quantity (" << order->quantity << ") must be greater than zero";
		throw invalid_argument(oss.str());
	}
	
	if (!isPriceTickAligned(order->price, orderSecurityTick)) {
		ostringstream oss;
		oss << "Order price (" << order->price << ") does not align with tick size (" << orderSecurityTick << ")";
		throw invalid_argument(oss.str());
	}
	
	matcher.matchOrder(order);
	auto newOrder = order;
	if (order->fulfilled != FULLY_FULFILLED && order->fulfilled != CANCELLED) {
		double priceBucket = getPriceBucket(order->price, this->securities[order->security]->getBucketSize());
		if (order->type == BUY) {
			if (buyOrders[order->security].find(priceBucket) == buyOrders[order->security].end())
			buyOrders[order->security][priceBucket] = std::make_shared<BuyBucket>();
			buyOrders[order->security][priceBucket]->bucket.push(newOrder);	
			buyOrders[order->security][priceBucket]->total_quantity += newOrder->quantity;
		}
		else {
			if (sellOrders[order->security].find(priceBucket) == sellOrders[order->security].end())
			sellOrders[order->security][priceBucket] = std::make_shared<SellBucket>();
			sellOrders[order->security][priceBucket]->bucket.push(newOrder);	
			sellOrders[order->security][priceBucket]->total_quantity += newOrder->quantity;
		}
	}
	allOrders.insert({newOrder->id, newOrder});
}

void Book::cancelOrder(const string& orderId) {
	try {
		auto order = orderLookup(orderId);

		if (order->fulfilled == CANCELLED || order->fulfilled == FULLY_FULFILLED) {
			return;
		}

		double bucketPrice = getPriceBucket(order->price, securities[order->security]->getBucketSize());
		if (order->type == BUY) {
			auto& bucket = buyOrders[order->security][bucketPrice];
			if (order->quantity > 0) {
				bucket->total_quantity -= order->quantity;
			}
		} else {
			auto& bucket = sellOrders[order->security][bucketPrice];
			if (order->quantity > 0) {
				bucket->total_quantity -= order->quantity;
			}
		}
		
		order->fulfilled = CANCELLED;
	} catch (const std::out_of_range &e) {
		logger->error("Order {} doesn't exist", orderId);
	}
}

void Book::modifyOrder(const string& orderId, double newQty, double newPrice) {
	try {
		auto order = orderLookup(orderId);
		
		// Check new quantity against the existing quantity
		if (newQty < order->quantity) {
			ostringstream oss;
			oss << "New quantity (" << newQty << ") cannot be less than the existing quantity (" << order->quantity << ")";
			throw invalid_argument(oss.str());
		}
		
		cancelOrder(orderId);
		shared_ptr<Order> modifiedOrder = std::make_shared<Order>(order->security, order->type, newQty, newPrice);
		
		try {
			insertOrder(modifiedOrder);
		} catch (const invalid_argument& arg) {
			logger->error("Order rejected: {}", arg.what());
		}
		
	} catch (const std::out_of_range &e) {
		logger->error("Order {} doesn't exist", orderId);
	}
}

shared_ptr<Order> Book::orderLookup(const string& orderId) {
	return allOrders.at(orderId);
}	
unordered_map<string, shared_ptr<Security>>& Book::getSecurities() {
	return securities;
}

unordered_map<string, map<double, shared_ptr<BuyBucket>>>& Book::getBuyOrders() {
	return buyOrders;
}

unordered_map<string, map<double, shared_ptr<SellBucket>>>& Book::getSellOrders() {
	return sellOrders;
}

void Book::cleanUpBuckets(shared_ptr<Order> order){
	if (order->type == BUY) {
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

void Book::printBuyOrders() {
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

void Book::printBuyOrdersFromAll() {
	if (buyOrders.empty()) {
		logger->info("# No Buy Orders");
		return;
	}	
	logger->info("# Buy Orders");
	logger->info("=============");
	for (auto& [securityString, order] : allOrders) {
		if (order->type == BUY && (order->fulfilled == NOT_FULFILLED || order->fulfilled == PARTIALLY_FULFILLED)) {
			order->print();
		}	
	}	
}	

void Book::printSellOrders() {
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

void Book::printSellOrdersFromAll() {
	if (sellOrders.empty()) {
		logger->info("No Sell Orders");
		return;
	}	
	logger->info("# Sell Orders");
	logger->info("=============");
	for (auto& [securityString, order] : allOrders) {
		if (order->type == SELL && (order->fulfilled == NOT_FULFILLED || order->fulfilled == PARTIALLY_FULFILLED)) {
			order->print();
		}	
	}	
}	
