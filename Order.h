#ifndef ORDER_H
#define ORDER_H

#include <string>
#include <queue>
#include <memory>

using std::string;
using std::shared_ptr;
using std::queue;

enum OrderType {
    BUY,
    SELL
};

enum Fulfilled {
    NOT_FULFILLED,
    PARTIALLY_FULFILLED,
    FULLY_FULFILLED,
    CANCELLED
};

class Order {
public:
    Order(string security, OrderType type, double quantity, double price);

    string id;
    string security;
    OrderType type;
    double quantity;
    double price;
    Fulfilled fulfilled;

    void print() const;
};

string generateUUID();

#endif // ORDER_H
