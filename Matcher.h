#ifndef MATCHER_H
#define MATCHER_H

#pragma once

#include "Math.h"
#include "Order.h"
#include "IMatcher.h"
#include <memory>

class IOrderBook;
class BuyBucket;
class SellBucket;

class Matcher : public IMatcher {
private:
    IOrderBook& book;

    void matchBuyOrder(std::shared_ptr<Order> order);
    void matchSellOrder(std::shared_ptr<Order> order);
    void matchBuyAgainstBucket(std::shared_ptr<Order>& buyOrder, std::shared_ptr<SellBucket>& bucket);
    void matchSellAgainstBucket(std::shared_ptr<Order>& sellOrder, std::shared_ptr<BuyBucket>& bucket);

public:
    Matcher(IOrderBook& book);

    void matchOrder(std::shared_ptr<Order> order) override;

};

#endif // MATCHER_H
