#ifndef PRICETIMEPRIORITYSTRATEGY_H
#define PRICETIMEPRIORITYSTRATEGY_H

#pragma once

#include "../headers/Math.h"
#include "IOrderMatchingStrategy.h"
#include <memory>

class IOrderBook;
class BuyBucket;
class SellBucket;

class PriceTimePriorityStrategy : public IOrderMatchingStrategy {
private:
    void matchBuyOrder(std::shared_ptr<Order> order, IOrderBook& book);
    void matchSellOrder(std::shared_ptr<Order> order, IOrderBook& book);
    void matchBuyAgainstBucket(std::shared_ptr<Order>& buyOrder, std::shared_ptr<SellBucket>& bucket);
    void matchSellAgainstBucket(std::shared_ptr<Order>& sellOrder, std::shared_ptr<BuyBucket>& bucket);

public:
    PriceTimePriorityStrategy() = default;

    void matchOrder(std::shared_ptr<Order> order, IOrderBook& book) override;

};

#endif // PRICETIMEPRIORITYSTRATEGY_H
