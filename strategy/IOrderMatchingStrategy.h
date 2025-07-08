#ifndef IORDERMATCHINGMATCHER_H
#define IORDERMATCHINGMATCHER_H

#include <memory>
class Order;
class IOrderBook;

class IOrderMatchingStrategy {
public:
    virtual void matchOrder(std::shared_ptr<Order> order, IOrderBook& book) = 0;
    virtual ~IOrderMatchingStrategy() = default;
};

#endif // IORDERMATCHINGMATCHER_H
