#ifndef MATCHER_H
#define MATCHER_H

#pragma once

#include "Math.h"
#include "Order.h"
#include <memory>

class Book;
class BuyBucket;
class SellBucket;

class Matcher {
private:
    Book& book;

    void matchBuyOrder(std::shared_ptr<Order> order);
    void matchSellOrder(std::shared_ptr<Order> order);
    void matchBuyAgainstBucket(std::shared_ptr<Order>& buyOrder, std::shared_ptr<SellBucket>& bucket);
    void matchSellAgainstBucket(std::shared_ptr<Order>& sellOrder, std::shared_ptr<BuyBucket>& bucket);

public:
    Matcher(Book& book);

    void matchOrder(std::shared_ptr<Order> order);

};

#endif // MATCHER_H
