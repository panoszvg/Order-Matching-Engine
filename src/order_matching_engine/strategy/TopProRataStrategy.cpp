#include "TopProRataStrategy.h"
#include "headers/Math.h"
#include "IOrderBook.h"
#include <vector>
#include <cmath>
#include <tuple>
#include <list>

void TopProRataStrategy::matchBuyOrder(Order& newOrder, IOrderBook& book) {
	auto& sellBucket = book.getSellOrders();
	if (sellBucket.empty()) return;
	matchBuyAgainstBucket(newOrder, sellBucket, book.getOrders(), book.getSecurity().getLotSize());
}

void TopProRataStrategy::matchSellOrder(Order& newOrder, IOrderBook& book) {
	auto& buyBucket = book.getBuyOrders();
	if (buyBucket.empty()) return;
	matchSellAgainstBucket(newOrder, buyBucket, book.getOrders(), book.getSecurity().getLotSize());
}

template<typename BucketT>
void TopProRataStrategy::applyFill(Order& incoming, Order& resting, const std::string& id,
                                    double tradedQty, BucketT& bucket, OrderMap& orders) {
	incoming.quantity     -= tradedQty;
	resting.quantity      -= tradedQty;
	bucket.total_quantity -= tradedQty;

	incoming.fulfilled = compareDoubles(incoming.quantity, 0.0) == 0 ? FULLY_FULFILLED : PARTIALLY_FULFILLED;
	resting.fulfilled  = compareDoubles(resting.quantity,  0.0) == 0 ? FULLY_FULFILLED : PARTIALLY_FULFILLED;

	if (resting.fulfilled == FULLY_FULFILLED) {
		orders.erase(id);
		bucket.erase(id);
	}
}

void TopProRataStrategy::matchBuyAgainstBucket(Order& buyOrder, SellBucket& bucket, OrderMap& orders, double lotSize) {
	while (!bucket.empty()
		&& buyOrder.fulfilled != FULLY_FULFILLED
		&& buyOrder.fulfilled != CANCELLED)
	{
		auto it = bucket.queue.begin();
		double levelPrice = it->price;
		if (compareDoubles(buyOrder.price, levelPrice) < 0) break;

		std::list<std::string> ids;
		double levelQuantity = 0.0;
		while (it != bucket.queue.end() && it->price == levelPrice) {
			levelQuantity += orders.at(it->id).quantity;
			ids.push_back(it->id);
			++it;
		}

		if (compareDoubles(levelQuantity, buyOrder.quantity) <= 0) {
			// fill all orders anyway (same as price time priority strategy)
			while (compareDoubles(levelQuantity, 0.0) > 0) {
				const std::string id = bucket.top().id;
				Order& sellOrder = orders.at(id);
				double tradedQty = std::min(buyOrder.quantity, sellOrder.quantity);
				applyFill(buyOrder, sellOrder, id, tradedQty, bucket, orders);
				levelQuantity -= tradedQty;
			}
		} else {
			auto firstOrderId = ids.front();
			Order& firstOrder = orders.at(firstOrderId);
			double tradedQty = std::min(PRIORITY_LOT, std::min(buyOrder.quantity, firstOrder.quantity));
			applyFill(buyOrder, firstOrder, firstOrderId, tradedQty, bucket, orders);
			levelQuantity -= tradedQty;

			if (firstOrder.fulfilled == FULLY_FULFILLED) {
				ids.pop_front();
			}

			if (buyOrder.fulfilled == FULLY_FULFILLED)
				return;

			if (ids.empty())
				continue;

			std::vector<double> shares;
			shares.reserve(ids.size());
			double sharesSum = 0;

			for (const auto& id : ids) {
				Order& order = orders.at(id);
				double rawShare = (order.quantity * buyOrder.quantity) / levelQuantity;
				double share = std::floor(rawShare / lotSize) * lotSize;
				sharesSum += share;
				shares.emplace_back(share);
			}

			int remainderLots = static_cast<int>(std::round((buyOrder.quantity - sharesSum) / lotSize));

			size_t idx = 0;
			for (const auto& id : ids) {
				Order& order = orders.at(id);
				double tradedQty = shares[idx++] + ((remainderLots-- > 0) ? lotSize : 0);
				applyFill(buyOrder, order, id, tradedQty, bucket, orders);
				levelQuantity -= tradedQty;
			}
		}
	}
}

void TopProRataStrategy::matchSellAgainstBucket(Order& sellOrder, BuyBucket& bucket, OrderMap& orders, double lotSize) {
	while (!bucket.empty()
		&& sellOrder.fulfilled != FULLY_FULFILLED
		&& sellOrder.fulfilled != CANCELLED)
	{
		auto it = bucket.queue.begin();
		double levelPrice = it->price;
		if (compareDoubles(sellOrder.price, levelPrice) > 0) break;

		std::list<std::string> ids;
		double levelQuantity = 0.0;
		while (it != bucket.queue.end() && it->price == levelPrice) {
			levelQuantity += orders.at(it->id).quantity;
			ids.push_back(it->id);
			++it;
		}

		if (compareDoubles(levelQuantity, sellOrder.quantity) <= 0) {
			// fill all orders anyway (same as price time priority strategy)
			while (compareDoubles(levelQuantity, 0.0) > 0) {
				const std::string id = bucket.top().id;
				Order& buyOrder = orders.at(id);
				double tradedQty = std::min(sellOrder.quantity, buyOrder.quantity);
				applyFill(sellOrder, buyOrder, id, tradedQty, bucket, orders);
				levelQuantity -= tradedQty;
			}
		} else {
			auto firstOrderId = ids.front();
			Order& firstOrder = orders.at(firstOrderId);
			double tradedQty = std::min(PRIORITY_LOT, std::min(sellOrder.quantity, firstOrder.quantity));
			applyFill(sellOrder, firstOrder, firstOrderId, tradedQty, bucket, orders);
			levelQuantity -= tradedQty;

			if (firstOrder.fulfilled == FULLY_FULFILLED) {
				ids.pop_front();
			}

			if (sellOrder.fulfilled == FULLY_FULFILLED)
				return;

			if (ids.empty())
				continue;

			std::vector<double> shares;
			shares.reserve(ids.size());
			double sharesSum = 0;

			for (const auto& id : ids) {
				Order& order = orders.at(id);
				double rawShare = (order.quantity * sellOrder.quantity) / levelQuantity;
				double share = std::floor(rawShare / lotSize) * lotSize;
				sharesSum += share;
				shares.emplace_back(share);
			}

			int remainderLots = static_cast<int>(std::round((sellOrder.quantity - sharesSum) / lotSize));

			size_t idx = 0;
			for (const auto& id : ids) {
				Order& order = orders.at(id);
				double tradedQty = shares[idx++] + ((remainderLots-- > 0) ? lotSize : 0);
				applyFill(sellOrder, order, id, tradedQty, bucket, orders);
				levelQuantity -= tradedQty;
			}
		}
	}
}

void TopProRataStrategy::matchOrder(Order& order, IOrderBook& book) {
	if (order.type == OrderType::BUY)
		matchBuyOrder(order, book);
	else
		matchSellOrder(order, book);
}
