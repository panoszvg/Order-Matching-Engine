#ifndef IORDERBOOK_H
#define IORDERBOOK_H

#include "Order.h"
#include "Security.h"
#include "headers/Math.h"
#include "headers/Buckets.h"
#include <map>
#include <memory>
#include <vector>
#include <unordered_map>

using std::map;
using std::shared_ptr;
using std::unordered_map;

class IOrderBook {
public:
    virtual void addSecurities(std::vector<shared_ptr<Security>>& securities) {
    virtual void insertOrder(std::shared_ptr<Order> order) = 0;
    virtual void cancelOrder(const std::string& id) = 0;
    virtual void modifyOrder(const string& orderId, double newQty, double newPrice) = 0;
    virtual std::shared_ptr<Order> orderLookup(const std::string& id) = 0;

    virtual shared_ptr<Security> getSecurity() = 0;
    virtual map<double, shared_ptr<BuyBucket>>& getBuyOrders() = 0;
    virtual map<double, shared_ptr<SellBucket>>& getSellOrders() = 0;

    virtual void cleanUpBuckets(shared_ptr<Order> order) {
        // Default : do nothing
    }
    virtual void printBuyOrders() {
        // Default : do nothing
    }
    virtual void printBuyOrdersFromAll() {
        // Default : do nothing
    }
    virtual void printSellOrders() {
        // Default : do nothing
    }
    virtual void printSellOrdersFromAll() {
        // Default : do nothing
    }
    virtual ~IOrderBook() = default;
};

#endif // IORDERBOOK_H