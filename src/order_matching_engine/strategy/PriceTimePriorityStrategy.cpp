#include "PriceTimePriorityStrategy.h"
#include "../headers/Buckets.h"
#include "../headers/Math.h"
#include "../IOrderBook.h"

void PriceTimePriorityStrategy::matchBuyOrder(std::shared_ptr<Order> newOrder, IOrderBook& book) {
	if (book.getSellOrders().empty()) return;

	auto& sellBuckets = book.getSellOrders();
	if (sellBuckets.empty()) return;

	double orderPriceBucket = getPriceBucket(newOrder->price, book.getSecurity().getBucketSize());

	auto endIt = sellBuckets.upper_bound(orderPriceBucket);

	for (auto it = sellBuckets.begin(); it != endIt; ++it) {
		auto& bucket = it->second;
		if (bucket->bucket.empty()) continue;

		matchBuyAgainstBucket(newOrder, bucket);
	}
}

void PriceTimePriorityStrategy::matchSellOrder(std::shared_ptr<Order> newOrder, IOrderBook& book) {
    if (book.getBuyOrders().empty()) return;

	auto& buyBuckets = book.getBuyOrders();
	if (buyBuckets.empty()) return;

	double orderPriceBucket = getPriceBucket(newOrder->price, book.getSecurity().getBucketSize());

	for (auto rIt = buyBuckets.rbegin(); rIt != buyBuckets.rend(); ++rIt) {
		if (rIt->first < orderPriceBucket)
			break;

		auto& bucket = rIt->second;
		if (bucket->bucket.empty()) continue;

		matchSellAgainstBucket(newOrder, bucket);
	}
}

void PriceTimePriorityStrategy::matchBuyAgainstBucket(std::shared_ptr<Order>& buyOrder, std::shared_ptr<SellBucket>& bucket) {
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


void PriceTimePriorityStrategy::matchSellAgainstBucket(std::shared_ptr<Order>& sellOrder, std::shared_ptr<BuyBucket>& bucket) {
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

void PriceTimePriorityStrategy::matchOrder(std::shared_ptr<Order> order, IOrderBook& book) {
    if (order->type == OrderType::BUY)
		matchBuyOrder(order, book);
	else matchSellOrder(order, book);
	book.cleanUpBuckets(order);
}
