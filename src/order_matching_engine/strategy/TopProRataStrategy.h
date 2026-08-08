#ifndef TOPPRORATASTRATEGY_H
#define TOPPRORATASTRATEGY_H

#pragma once

#include "headers/Math.h"
#include "headers/Buckets.h"
#include "IOrderMatchingStrategy.h"

class IOrderBook;

class TopProRataStrategy : public IOrderMatchingStrategy {
private:
	const double PRIORITY_LOT = 5.0;
	void matchBuyOrder(Order& order, IOrderBook& book);
	void matchSellOrder(Order& order, IOrderBook& book);
	template<typename BucketT>
	void applyFill(Order& incoming, Order& resting, const std::string& id,
				   double tradedQty, BucketT& bucket, OrderMap& orders);
	void matchBuyAgainstBucket(Order& buyOrder, SellBucket& bucket, OrderMap& orders, double lotSize);
	void matchSellAgainstBucket(Order& sellOrder, BuyBucket& bucket, OrderMap& orders, double lotSize);

public:
	TopProRataStrategy() = default;
	void matchOrder(Order& order, IOrderBook& book) override;
};

#endif // TOPPRORATASTRATEGY_H
