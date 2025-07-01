#include "Book.h"
#include <string>

Book::Book() = default;

void Book::cleanUpBuckets(shared_ptr<Order> order){
	if (order->type == OrderType::BUY) {
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

inline double Book::getPriceBucket(double price, double bucket_size) {
	return floor(price / bucket_size) * bucket_size;
}

inline int Book::compareDoubles(double a, double b) {
	if (fabs(a - b) < EPSILON) return 0;
	return (a < b) ? -1 : 1;
}

inline bool Book::isPriceTickAligned(double price, double tick) {
	double ratio = price / tick;
	double rounded = round(ratio);
	return fabs(ratio - rounded) < EPSILON;
}


void Book::addSecurities(vector<shared_ptr<Security>>& securities) {
	for (auto& security : securities) {
		this->securities[security->getSymbol()] = security;
	}
}

void Book::printSellOrders() const {
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

void Book::printSellOrdersFromAll() const {
	if (sellOrders.empty()) {
		logger->info("No Sell Orders");
		return;
	}
	logger->info("# Sell Orders");
	logger->info("=============");
	for (auto& [securityString, order] : allOrders) {
		if (order->type == OrderType::SELL && (order->fulfilled == NOT_FULFILLED || order->fulfilled == PARTIALLY_FULFILLED)) {
			order->print();
		}
	}
}

void Book::printBuyOrders() const {
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

void Book::printBuyOrdersFromAll() const {
	if (buyOrders.empty()) {
		logger->info("# No Buy Orders");
		return;
	}
	logger->info("# Buy Orders");
	logger->info("=============");
	for (auto& [securityString, order] : allOrders) {
		if (order->type == OrderType::BUY && (order->fulfilled == NOT_FULFILLED || order->fulfilled == PARTIALLY_FULFILLED)) {
			order->print();
		}
	}
}

void Book::matchBuyAgainstBucket(shared_ptr<Order>& buyOrder, shared_ptr<SellBucket>& bucket) {
	while (!bucket->bucket.empty()
		&& buyOrder->fulfilled != FULLY_FULFILLED
		&& buyOrder->fulfilled != CANCELLED)
	{
		auto sellOrder = bucket->bucket.top();
		if (sellOrder->fulfilled == FULLY_FULFILLED
		 || sellOrder->fulfilled == CANCELLED)
		{
			bucket->bucket.pop();
			continue;
		}

		// Stop if the sell price is too high
		if (compareDoubles(buyOrder->price, sellOrder->price) < 0)
			break;

		// Determine how much can be traded
		double tradedQty = std::min(buyOrder->quantity, sellOrder->quantity);

		// Subtract from both orders
		buyOrder->quantity -= tradedQty;
		sellOrder->quantity -= tradedQty;

		// Update fulfillment status for buy order
		if (compareDoubles(buyOrder->quantity, 0.0) == 0)
			buyOrder->fulfilled = FULLY_FULFILLED;
		else
			buyOrder->fulfilled = PARTIALLY_FULFILLED;

		// Update fulfillment status for sell order
		if (compareDoubles(sellOrder->quantity, 0.0) == 0) {
			sellOrder->fulfilled = FULLY_FULFILLED;
			bucket->bucket.pop();
		} else {
			sellOrder->fulfilled = PARTIALLY_FULFILLED;
		}

		// Reduce total quantity in the bucket
		bucket->total_quantity -= tradedQty;
	}
}

void Book::matchBuyOrder(shared_ptr<Order> newOrder) {
	if (sellOrders.empty()) return;

	auto& sellBuckets = sellOrders[newOrder->security];
	if (sellBuckets.empty()) return;

	double orderPriceBucket = getPriceBucket(newOrder->price, securities[newOrder->security]->getBucketSize());

	auto endIt = sellBuckets.upper_bound(orderPriceBucket);

	for (auto it = sellBuckets.begin(); it != endIt; ++it) {
		auto& bucket = it->second;
		if (bucket->bucket.empty()) continue;

		matchBuyAgainstBucket(newOrder, bucket);
	}
}


void Book::matchSellAgainstBucket(shared_ptr<Order>& sellOrder, shared_ptr<BuyBucket>& bucket) {
	while (!bucket->bucket.empty()
		&& sellOrder->fulfilled != FULLY_FULFILLED
		&& sellOrder->fulfilled != CANCELLED)
	{
		auto buyOrder = bucket->bucket.top();
		if (buyOrder->fulfilled == FULLY_FULFILLED
		 || buyOrder->fulfilled == CANCELLED)
		{
			bucket->bucket.pop();
			continue;
		}
		// Stop if the buy price is too low
		if (compareDoubles(sellOrder->price, buyOrder->price) > 0)
			break;

		// Determine how much can be traded
		double tradedQty = std::min(sellOrder->quantity, buyOrder->quantity);

		// Subtract from both orders
		sellOrder->quantity -= tradedQty;
		buyOrder->quantity -= tradedQty;

		// Update fulfillment status for sell order
		if (compareDoubles(sellOrder->quantity, 0.0) == 0)
			sellOrder->fulfilled = FULLY_FULFILLED;
		else
			sellOrder->fulfilled = PARTIALLY_FULFILLED;

		// Update fulfillment status for buy order
		if (compareDoubles(buyOrder->quantity, 0.0) == 0) {
			buyOrder->fulfilled = FULLY_FULFILLED;
			bucket->bucket.pop();
		} else {
			buyOrder->fulfilled = PARTIALLY_FULFILLED;
		}

		// Reduce total quantity in the bucket
		bucket->total_quantity -= tradedQty;
	}
}

void Book::matchSellOrder(shared_ptr<Order> newOrder) {
	if (buyOrders.empty()) return;

	auto& buyBuckets = buyOrders[newOrder->security];
	if (buyBuckets.empty()) return;

	double orderPriceBucket = getPriceBucket(newOrder->price, securities[newOrder->security]->getBucketSize());

	for (auto rIt = buyBuckets.rbegin(); rIt != buyBuckets.rend(); ++rIt) {
		if (rIt->first < orderPriceBucket)
			break;

		auto& bucket = rIt->second;
		if (bucket->bucket.empty()) continue;

		matchSellAgainstBucket(newOrder, bucket);
	}
}

shared_ptr<Order> Book::orderLookup(const string& orderId) {
	return allOrders.at(orderId);
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
			std::ostringstream oss;
			oss << "New quantity (" << newQty << ") cannot be less than the existing quantity (" << order->quantity << ")";
			throw std::invalid_argument(oss.str());
		}
		
		cancelOrder(orderId);
		shared_ptr<Order> modifiedOrder = std::make_shared<Order>(order->security, order->type, newQty, newPrice);

		try {
			insertOrder(modifiedOrder);
		} catch (const std::invalid_argument& arg) {
			logger->error("Order rejected: {}", arg.what());
		}

	} catch (const std::out_of_range &e) {
		logger->error("Order {} doesn't exist", orderId);
	}
}

void Book::matchOrder(shared_ptr<Order> order) {
	if (order->type == OrderType::BUY)
		matchBuyOrder(order);
	else matchSellOrder(order);
	cleanUpBuckets(order);
}

void Book::insertOrder(shared_ptr<Order> order) {
	if (this->securities.find(order->security) == this->securities.end())
		return;

	auto orderSecurity = this->securities[order->security];
	double orderSecurityTick = orderSecurity->getTickSize();
	
	if (compareDoubles(order->price, 0.0) <= 0) {
		std::ostringstream oss;
		oss << "Price (" << order->price << ") must be greater than zero";
		throw std::invalid_argument(oss.str());
	}
	
	if (compareDoubles(order->quantity, 0.0) <= 0) {
		std::ostringstream oss;
		oss << "Quantity (" << order->quantity << ") must be greater than zero";
		throw std::invalid_argument(oss.str());
	}

	if (!isPriceTickAligned(order->price, orderSecurityTick)) {
		std::ostringstream oss;
		oss << "Order price (" << order->price << ") does not align with tick size (" << orderSecurityTick << ")";
		throw std::invalid_argument(oss.str());
	}

	matchOrder(order);
	auto newOrder = order;
	if (order->fulfilled != FULLY_FULFILLED && order->fulfilled != CANCELLED) {
		double priceBucket = getPriceBucket(order->price, this->securities[order->security]->getBucketSize());
		if (order->type == OrderType::BUY) {
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
