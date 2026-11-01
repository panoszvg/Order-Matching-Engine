#pragma once

#include "Book.h"
#include "strategy/PriceTimePriorityStrategy.h"
#include "Security.h"
#include <memory>
#include <string>

inline std::shared_ptr<Book> makeBook(
    const std::string& symbol = "TST",
    double tick = 0.1,
    double lotSize = 0.5) {
    return std::make_shared<Book>(
        std::make_unique<PriceTimePriorityStrategy>(),
        std::make_unique<Security>(symbol, tick, lotSize)
    );
}

template<typename StrategyT>
inline std::shared_ptr<Book> makeBookWithStrategy(
    const std::string& symbol = "TST",
    double tick = 0.1,
    double lotSize = 0.5) {
    return std::make_shared<Book>(
        std::make_unique<StrategyT>(),
        std::make_unique<Security>(symbol, tick, lotSize)
    );
}

template<typename BucketT>
inline double restingQuantity(const BucketT& bucket, const OrderMap& orders) {
    double total = 0.0;
    for (const auto& entry : bucket.queue)
        total += orders.at(entry.id).quantity;
    return total;
}

inline double restingBuyQuantity(Book& book)  { return restingQuantity(book.getBuyOrders(),  book.getOrders()); }
inline double restingSellQuantity(Book& book) { return restingQuantity(book.getSellOrders(), book.getOrders()); }
