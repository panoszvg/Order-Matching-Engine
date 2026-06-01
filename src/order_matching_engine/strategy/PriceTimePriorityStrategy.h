#ifndef PRICETIMEPRIORITYSTRATEGY_H
#define PRICETIMEPRIORITYSTRATEGY_H

#pragma once

#include "headers/Math.h"
#include "headers/Buckets.h"
#include "IOrderMatchingStrategy.h"

class IOrderBook;

class PriceTimePriorityStrategy : public IOrderMatchingStrategy {
private:
	void matchBuyOrder(Order& order, IOrderBook& book);
	void matchSellOrder(Order& order, IOrderBook& book);
	void matchBuyAgainstBucket(Order& buyOrder, SellBucket& bucket, OrderMap& orders);
	void matchSellAgainstBucket(Order& sellOrder, BuyBucket& bucket, OrderMap& orders);

public:
	PriceTimePriorityStrategy() = default;
	void matchOrder(Order& order, IOrderBook& book) override;
};

#endif // PRICETIMEPRIORITYSTRATEGY_H
