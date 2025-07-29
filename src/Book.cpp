#include "Book.h"
#include "strategy/PriceTimePriorityStrategy.h"
#include <string>

Book::Book(std::shared_ptr<IOrderMatchingStrategy> matcher, shared_ptr<Security> security) :
	matcher(std::move(matcher)), security(std::move(security)) {}

void Book::insertOrderUnlocked(shared_ptr<Order> order) {
	double orderSecurityTick = security->getTickSize();
	
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

	order->timestamp = std::chrono::system_clock::now();
	logger->info("Insert order {} @ {} μs | Price: {} | Qty: {}",
				  order->id, order->getMicroTimestamp(), order->price, order->quantity);
	matcher->matchOrder(order, *this);
	auto newOrder = order;
	if (order->fulfilled != FULLY_FULFILLED && order->fulfilled != CANCELLED) {
		double priceBucket = getPriceBucket(order->price, security->getBucketSize());
		if (order->type == BUY) {
			if (buyOrders.find(priceBucket) == buyOrders.end())
			buyOrders[priceBucket] = std::make_shared<BuyBucket>();
			buyOrders[priceBucket]->bucket.push(newOrder);	
			buyOrders[priceBucket]->total_quantity += newOrder->quantity;
		}
		else {
			if (sellOrders.find(priceBucket) == sellOrders.end())
			sellOrders[priceBucket] = std::make_shared<SellBucket>();
			sellOrders[priceBucket]->bucket.push(newOrder);	
			sellOrders[priceBucket]->total_quantity += newOrder->quantity;
		}
	}
	allOrders.insert({newOrder->id, newOrder});
}

void Book::cancelOrderUnlocked(const string& orderId) {
	try {
		auto order = orderLookupUnlocked(orderId);

		if (order->fulfilled == CANCELLED || order->fulfilled == FULLY_FULFILLED) {
			return;
		}

		double bucketPrice = getPriceBucket(order->price, security->getBucketSize());
		if (order->type == BUY) {
			auto& bucket = buyOrders[bucketPrice];
			if (order->quantity > 0) {
				bucket->total_quantity -= order->quantity;
			}
		} else {
			auto& bucket = sellOrders[bucketPrice];
			if (order->quantity > 0) {
				bucket->total_quantity -= order->quantity;
			}
		}
		
		order->fulfilled = CANCELLED;
	} catch (const std::out_of_range &e) {
		logger->error("Order {} doesn't exist", orderId);
	}
}

shared_ptr<Order> Book::orderLookupUnlocked(const std::string& orderId) {
    return allOrders.at(orderId);
}

void Book::modifyOrder(const string& orderId, double newQty, double newPrice) {
	std::lock_guard<std::mutex> lock(bookMutex);
	try {
		auto order = orderLookupUnlocked(orderId);
		
		// Check new quantity against the existing quantity
		if (newQty < order->quantity) {
			ostringstream oss;
			oss << "New quantity (" << newQty << ") cannot be less than the existing quantity (" << order->quantity << ")";
			throw invalid_argument(oss.str());
		}
		
		cancelOrderUnlocked(orderId);
		shared_ptr<Order> modifiedOrder = std::make_shared<Order>(order->security, order->type, newQty, newPrice);
		
		try {
			insertOrderUnlocked(modifiedOrder);
		} catch (const invalid_argument& arg) {
			logger->error("Order rejected: {}", arg.what());
		}
		
	} catch (const std::out_of_range &e) {
		logger->error("Order {} doesn't exist", orderId);
	}
}

void Book::insertOrder(shared_ptr<Order> order) {
	std::lock_guard<std::mutex> lock(bookMutex);
	insertOrderUnlocked(order);
}

void Book::cancelOrder(const string& orderId) {
	std::lock_guard<std::mutex> lock(bookMutex);
	cancelOrderUnlocked(orderId);
}

void Book::setMatchingStrategy(std::shared_ptr<IOrderMatchingStrategy> newMatcher) {
	matcher = std::move(newMatcher);
}

void Book::setSecurity(std::shared_ptr<Security> security) {
	strategy = std::move(strategy);
}

shared_ptr<Order> Book::orderLookup(const std::string& orderId) {
    std::lock_guard<std::mutex> lock(bookMutex);
    return orderLookupUnlocked(orderId);
}

shared_ptr<Security> Book::getSecurity() {
	return this->security;
}

map<double, shared_ptr<BuyBucket>>& Book::getBuyOrders() {
	return buyOrders;
}

map<double, shared_ptr<SellBucket>>& Book::getSellOrders() {
	return sellOrders;
}

void Book::cleanUpBuckets(shared_ptr<Order> order){
	if (order->type == BUY) {
		auto& book = sellOrders;
		for (auto it = book.begin(); it != book.end(); ) {
			if (it->second->bucket.empty())
				it = book.erase(it);
			else
				++it;
			}
		}
	else {
		auto& book = buyOrders;
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
	for (auto [price, bucketContainer] : buyOrders) {
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
	for (auto [price, bucketContainer] : sellOrders) {
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

void Book::exportSnapshot() const {
	using json = nlohmann::json;

	json j;
	j["symbol"] = security->getSymbol();

	j["bids"] = json::array();
	for (auto it = buyOrders.rbegin(); it != buyOrders.rend(); ++it) {
		double price = it->first;
		const auto& bucket = it->second;
		if (!bucket || bucket->total_quantity <= 0)
			continue;

		j["bids"].push_back({
			{"price", round(price)},
			{"quantity", round(bucket->total_quantity)}
		});
	}

	j["asks"] = json::array();
	for (const auto& [price, bucket] : sellOrders) {
		if (!bucket || bucket->total_quantity <= 0)
			continue;

		j["asks"].push_back({
			{"price", round(price)},
			{"quantity", round(bucket->total_quantity)}
		});
	}

	std::filesystem::create_directory("snapshots");
	std::string filename = "snapshots/book_" + security->getSymbol() + ".json";
	std::ofstream out(filename);
	if (!out.is_open()) {
		spdlog::error("Failed to write snapshot for {} to {}", security->getSymbol(), filename);
		return;
	}

	out << j.dump(2);
	logger->info("Exported snapshot to {}", filename);
}

