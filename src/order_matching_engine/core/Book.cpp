#include "Book.h"
#include "strategy/PriceTimePriorityStrategy.h"
#include <string>

Book::Book(unique_ptr<IOrderMatchingStrategy> matcher, unique_ptr<Security> security) :
	matcher(std::move(matcher)), security(std::move(security)) {}

void Book::insertOrderUnlocked(Order& order) {
	double orderSecurityTick = security->getTickSize();

	if (compareDoubles(order.price, 0.0) <= 0) {
		ostringstream oss;
		oss << "Price (" << order.price << ") must be greater than zero";
		throw invalid_argument(oss.str());
	}

	if (compareDoubles(order.quantity, 0.0) <= 0) {
		ostringstream oss;
		oss << "Quantity (" << order.quantity << ") must be greater than zero";
		throw invalid_argument(oss.str());
	}

	if (!isPriceTickAligned(order.price, orderSecurityTick)) {
		ostringstream oss;
		oss << "Order price (" << order.price << ") does not align with tick size (" << orderSecurityTick << ")";
		throw invalid_argument(oss.str());
	}

	if (allOrders.count(order.id))
		throw invalid_argument("Duplicate order ID: " + order.id);

	allOrders.insert({order.id, order});
	Order& stored = allOrders.at(order.id);

	matcher->matchOrder(stored, *this);

	if (stored.fulfilled == FULLY_FULFILLED || stored.fulfilled == CANCELLED) {
		allOrders.erase(order.id);
	} else if (stored.type == BUY) {
		buyOrders.push(BuyEntry{stored.price, stored.getMicroTimestamp(), stored.id});
		buyOrders.total_quantity += stored.quantity;
	} else {
		sellOrders.push(SellEntry{stored.price, stored.getMicroTimestamp(), stored.id});
		sellOrders.total_quantity += stored.quantity;
	}
}

void Book::cancelOrderUnlocked(const string& orderId) {
	auto it = allOrders.find(orderId);
	if (it == allOrders.end())
		throw invalid_argument("Cancel failed: order " + orderId + " does not exist");

	Order& order = it->second;
	if (order.fulfilled == CANCELLED || order.fulfilled == FULLY_FULFILLED)
		return;

	if (order.type == BUY) {
		buyOrders.total_quantity -= order.quantity;
		buyOrders.erase(orderId);
	} else {
		sellOrders.total_quantity -= order.quantity;
		sellOrders.erase(orderId);
	}

	allOrders.erase(it);
}

Order& Book::orderLookupUnlocked(const std::string& orderId) {
	return allOrders.at(orderId);
}

void Book::modifyOrder(const string& orderId, double newQty, double newPrice) {
	std::lock_guard<std::mutex> lock(bookMutex);
	try {
		auto& order = orderLookupUnlocked(orderId);

		double filledQty = order.originalQuantity - order.quantity;
		if (compareDoubles(newQty, filledQty) <= 0) {
			ostringstream oss;
			oss << "New quantity (" << newQty << ") must be greater than already filled quantity (" << filledQty << ")";
			throw invalid_argument(oss.str());
		}

		std::string sec  = order.security;
		OrderType type = order.type;

		cancelOrderUnlocked(orderId);

		auto modifiedOrder = std::make_unique<Order>(sec, type, newQty - filledQty, newQty, newPrice);

		try {
			insertOrderUnlocked(*modifiedOrder);
		} catch (const invalid_argument& arg) {
			logger->error("Order rejected: {}", arg.what());
			throw;
		}

	} catch (const std::out_of_range& e) {
		logger->error("Order {} doesn't exist", orderId);
		throw;
	}
}

void Book::insertOrder(Order& order) {
	std::lock_guard<std::mutex> lock(bookMutex);
	insertOrderUnlocked(order);
}

void Book::cancelOrder(const string& orderId) {
	std::lock_guard<std::mutex> lock(bookMutex);
	cancelOrderUnlocked(orderId);
}

void Book::setMatchingStrategy(std::unique_ptr<IOrderMatchingStrategy> newMatcher) {
	matcher = std::move(newMatcher);
}

void Book::setSecurity(std::unique_ptr<Security> sec) {
	security = std::move(sec);
}

Order& Book::orderLookup(const std::string& orderId) {
	std::lock_guard<std::mutex> lock(bookMutex);
	return orderLookupUnlocked(orderId);
}

Security& Book::getSecurity() {
	return *(this->security);
}

OrderMap& Book::getOrders() {
	return allOrders;
}

BuyBucket& Book::getBuyOrders() {
	return buyOrders;
}

SellBucket& Book::getSellOrders() {
	return sellOrders;
}

void Book::printBuyOrders() {
	if (buyOrders.empty()) {
		logger->info("# No Buy Orders");
		return;
	}
	logger->info("# Buy Orders");
	logger->info("=============");
	logger->info("  total_quantity: {:.6}", buyOrders.total_quantity);
	for (const auto& entry : buyOrders.queue) {
		allOrders.at(entry.id).print();
	}
}

void Book::printBuyOrdersFromAll() {
	if (buyOrders.empty()) {
		logger->info("# No Buy Orders");
		return;
	}
	logger->info("# Buy Orders");
	logger->info("=============");
	for (auto& [id, order] : allOrders) {
		if (order.type == BUY) order.print();
	}
}

void Book::printSellOrders() {
	if (sellOrders.empty()) {
		logger->info("# No Sell Orders");
		return;
	}
	logger->info("# Sell Orders");
	logger->info("=============");
	logger->info("  total_quantity: {:.6}", sellOrders.total_quantity);
	for (const auto& entry : sellOrders.queue) {
		allOrders.at(entry.id).print();
	}
}

void Book::printSellOrdersFromAll() {
	if (sellOrders.empty()) {
		logger->info("# No Sell Orders");
		return;
	}
	logger->info("# Sell Orders");
	logger->info("=============");
	for (auto& [id, order] : allOrders) {
		if (order.type == SELL) order.print();
	}
}

void Book::exportSnapshot() const {
	auto jsonBook = toJson();

	std::filesystem::create_directory("snapshots");
	std::string timestamp = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
	std::string filename = "snapshots/book_" + security->getSymbol() + "_" + timestamp + ".json";
	std::ofstream out(filename);
	if (!out.is_open()) {
		spdlog::error("Failed to write snapshot for {} to {}", security->getSymbol(), filename);
		return;
	}

	out << jsonBook.dump(2);
	logger->info("Exported snapshot to {}", filename);
}

json Book::toJson() const {
	json j;
	j["symbol"] = security->getSymbol();
	double tick = security->getTickSize();

	j["bids"] = json::array();
	for (const auto& entry : buyOrders.queue) {
		const Order& o = allOrders.at(entry.id);
		j["bids"].push_back({
			{"price",    round(o.price / tick) * tick},
			{"quantity", o.quantity}
		});
	}

	j["asks"] = json::array();
	for (const auto& entry : sellOrders.queue) {
		const Order& o = allOrders.at(entry.id);
		j["asks"].push_back({
			{"price",    round(o.price / tick) * tick},
			{"quantity", o.quantity}
		});
	}

	return j;
}
