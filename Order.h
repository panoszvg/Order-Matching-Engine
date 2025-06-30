#ifndef ORDER_H
#define ORDER_H

#include <string>
#include <queue>
#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>

using std::string;
using std::shared_ptr;
using std::queue;

extern std::shared_ptr<spdlog::logger> logger;

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

class PriceBucket {
public:
    explicit PriceBucket(double price);

    double price;
    queue<shared_ptr<Order>> bucket;
};

string generateUUID();

#endif // ORDER_H
