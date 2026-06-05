#include "PriceTimePriorityStrategy.h"
#include "headers/Math.h"
#include "IOrderBook.h"

void PriceTimePriorityStrategy::matchBuyOrder(Order& newOrder, IOrderBook& book) {
	auto& sellBucket = book.getSellOrders();
	if (sellBucket.queue.empty()) return;
	matchBuyAgainstBucket(newOrder, sellBucket, book.getOrders());
}

void PriceTimePriorityStrategy::matchSellOrder(Order& newOrder, IOrderBook& book) {
	auto& buyBucket = book.getBuyOrders();
	if (buyBucket.queue.empty()) return;
	matchSellAgainstBucket(newOrder, buyBucket, book.getOrders());
}

void PriceTimePriorityStrategy::matchBuyAgainstBucket(Order& buyOrder, SellBucket& bucket, OrderMap& orders) {
	while (!bucket.queue.empty()
		&& buyOrder.fulfilled != FULLY_FULFILLED
		&& buyOrder.fulfilled != CANCELLED)
	{
		Order& sellOrder = orders.at(bucket.queue.top());
		if (sellOrder.fulfilled == FULLY_FULFILLED || sellOrder.fulfilled == CANCELLED) {
			bucket.queue.pop();
			continue;
		}

		if (compareDoubles(buyOrder.price, sellOrder.price) < 0) break;

		double tradedQty = std::min(buyOrder.quantity, sellOrder.quantity);

		buyOrder.quantity     -= tradedQty;
		sellOrder.quantity    -= tradedQty;
		bucket.total_quantity -= tradedQty;

		buyOrder.fulfilled  = compareDoubles(buyOrder.quantity,  0.0) == 0 ? FULLY_FULFILLED : PARTIALLY_FULFILLED;
		sellOrder.fulfilled = compareDoubles(sellOrder.quantity, 0.0) == 0 ? FULLY_FULFILLED : PARTIALLY_FULFILLED;

		if (sellOrder.fulfilled == FULLY_FULFILLED)
			bucket.queue.pop();
	}
}

void PriceTimePriorityStrategy::matchSellAgainstBucket(Order& sellOrder, BuyBucket& bucket, OrderMap& orders) {
	while (!bucket.queue.empty()
		&& sellOrder.fulfilled != FULLY_FULFILLED
		&& sellOrder.fulfilled != CANCELLED)
	{
		Order& buyOrder = orders.at(bucket.queue.top());
		if (buyOrder.fulfilled == FULLY_FULFILLED || buyOrder.fulfilled == CANCELLED) {
			bucket.queue.pop();
			continue;
		}

		if (compareDoubles(sellOrder.price, buyOrder.price) > 0) break;

		double tradedQty = std::min(sellOrder.quantity, buyOrder.quantity);

		sellOrder.quantity    -= tradedQty;
		buyOrder.quantity     -= tradedQty;
		bucket.total_quantity -= tradedQty;

		sellOrder.fulfilled = compareDoubles(sellOrder.quantity, 0.0) == 0 ? FULLY_FULFILLED : PARTIALLY_FULFILLED;
		buyOrder.fulfilled  = compareDoubles(buyOrder.quantity,  0.0) == 0 ? FULLY_FULFILLED : PARTIALLY_FULFILLED;

		if (buyOrder.fulfilled == FULLY_FULFILLED)
			bucket.queue.pop();
	}
}

void PriceTimePriorityStrategy::matchOrder(Order& order, IOrderBook& book) {
	if (order.type == OrderType::BUY)
		matchBuyOrder(order, book);
	else
		matchSellOrder(order, book);
}
