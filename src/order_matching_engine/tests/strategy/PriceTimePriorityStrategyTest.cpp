#include <gtest/gtest.h>
#include "TestHelpers.h"
#include <chrono>

// All strategy tests run through Book so the full insert/match pipeline is exercised.
// Fully-filled orders are erased from allOrders: count(id)==0 means filled.
// Unfilled/partial orders are still in allOrders and can be inspected via getOrders().at(id).

TEST(Strategy, BuyMatchesCheapestSell) {
    auto book = makeBook();
    auto expensive = Order("TST", SELL, 5.0, 102.0);
    auto cheap     = Order("TST", SELL, 5.0, 100.0);
    book->insertOrder(expensive);
    book->insertOrder(cheap);

    auto buy = Order("TST", BUY, 5.0, 102.0);
    book->insertOrder(buy);

    EXPECT_EQ(book->getOrders().count(cheap.id),    0u);  // matched and erased
    EXPECT_EQ(book->getOrders().at(expensive.id).fulfilled, NOT_FULFILLED);
    EXPECT_EQ(book->getOrders().count(buy.id),      0u);  // matched and erased
}

TEST(Strategy, SellMatchesHighestBid) {
    auto book = makeBook();
    auto low_bid  = Order("TST", BUY, 5.0, 99.0);
    auto high_bid = Order("TST", BUY, 5.0, 101.0);
    book->insertOrder(low_bid);
    book->insertOrder(high_bid);

    auto sell = Order("TST", SELL, 5.0, 99.0);
    book->insertOrder(sell);

    EXPECT_EQ(book->getOrders().count(high_bid.id), 0u);  // matched and erased
    EXPECT_EQ(book->getOrders().at(low_bid.id).fulfilled, NOT_FULFILLED);
    EXPECT_EQ(book->getOrders().count(sell.id),     0u);  // matched and erased
}

TEST(Strategy, NoCross_NoMatch) {
    auto book = makeBook();
    auto sell = Order("TST", SELL, 5.0, 101.0);
    auto buy  = Order("TST", BUY,  5.0, 100.0);
    book->insertOrder(sell);
    book->insertOrder(buy);

    EXPECT_EQ(book->getOrders().at(sell.id).fulfilled, NOT_FULFILLED);
    EXPECT_EQ(book->getOrders().at(buy.id).fulfilled,  NOT_FULFILLED);
}

TEST(Strategy, PartialFill_UpdatesRemainingQuantity) {
    auto book = makeBook();
    auto sell = Order("TST", SELL, 10.0, 100.0);
    book->insertOrder(sell);

    auto buy = Order("TST", BUY, 6.0, 100.0);
    book->insertOrder(buy);

    // buy fully filled → erased
    EXPECT_EQ(book->getOrders().count(buy.id), 0u);
    // sell partially filled → still in book
    EXPECT_EQ(book->getOrders().at(sell.id).fulfilled, PARTIALLY_FULFILLED);
    EXPECT_DOUBLE_EQ(book->getOrders().at(sell.id).quantity, 4.0);
}

TEST(Strategy, TimePriority_EarlierSellMatchedFirst) {
    auto book = makeBook();

    auto sell1 = Order("TST", SELL, 5.0, 100.0);
    sell1.timestamp = std::chrono::system_clock::time_point(std::chrono::microseconds(1000));
    auto sell2 = Order("TST", SELL, 5.0, 100.0);
    sell2.timestamp = std::chrono::system_clock::time_point(std::chrono::microseconds(2000));

    book->insertOrder(sell1);
    book->insertOrder(sell2);

    auto buy = Order("TST", BUY, 5.0, 100.0);
    book->insertOrder(buy);

    EXPECT_EQ(book->getOrders().count(sell1.id), 0u);  // earlier → matched and erased
    EXPECT_EQ(book->getOrders().at(sell2.id).fulfilled, NOT_FULFILLED);
    EXPECT_EQ(book->getOrders().count(buy.id),   0u);  // fully filled → erased
}

TEST(Strategy, TimePriority_EarlierBuyMatchedFirst) {
    auto book = makeBook();

    auto buy1 = Order("TST", BUY, 5.0, 100.0);
    buy1.timestamp = std::chrono::system_clock::time_point(std::chrono::microseconds(1000));
    auto buy2 = Order("TST", BUY, 5.0, 100.0);
    buy2.timestamp = std::chrono::system_clock::time_point(std::chrono::microseconds(2000));

    book->insertOrder(buy1);
    book->insertOrder(buy2);

    auto sell = Order("TST", SELL, 5.0, 100.0);
    book->insertOrder(sell);

    EXPECT_EQ(book->getOrders().count(buy1.id), 0u);  // earlier → matched and erased
    EXPECT_EQ(book->getOrders().at(buy2.id).fulfilled, NOT_FULFILLED);
    EXPECT_EQ(book->getOrders().count(sell.id), 0u);  // fully filled → erased
}

TEST(Strategy, SkipsCancelledRestingOrders) {
    auto book = makeBook();

    auto sell1 = Order("TST", SELL, 5.0, 100.0);
    sell1.timestamp = std::chrono::system_clock::time_point(std::chrono::microseconds(1000));
    auto sell2 = Order("TST", SELL, 5.0, 100.0);
    sell2.timestamp = std::chrono::system_clock::time_point(std::chrono::microseconds(2000));

    book->insertOrder(sell1);
    book->insertOrder(sell2);

    book->cancelOrder(sell1.id);  // sell1 would match first by timestamp

    auto buy = Order("TST", BUY, 5.0, 100.0);
    book->insertOrder(buy);

    // sell1 was cancelled → erased; sell2 matched buy → erased; buy fully filled → erased
    EXPECT_EQ(book->getOrders().count(sell1.id), 0u);
    EXPECT_EQ(book->getOrders().count(sell2.id), 0u);
    EXPECT_EQ(book->getOrders().count(buy.id),   0u);
    EXPECT_TRUE(book->getSellOrders().empty());
    EXPECT_TRUE(book->getBuyOrders().empty());
}

TEST(Strategy, BuyPriceExactlyAtAsk_Matches) {
    auto book = makeBook();
    auto sell = Order("TST", SELL, 5.0, 100.0);
    book->insertOrder(sell);

    auto buy = Order("TST", BUY, 5.0, 100.0);
    book->insertOrder(buy);

    EXPECT_EQ(book->getOrders().count(sell.id), 0u);
    EXPECT_EQ(book->getOrders().count(buy.id),  0u);
}

TEST(Strategy, BuyPriceAboveAsk_MatchesAtAsk) {
    auto book = makeBook();
    auto sell = Order("TST", SELL, 5.0, 100.0);
    book->insertOrder(sell);

    auto buy = Order("TST", BUY, 5.0, 101.0);
    book->insertOrder(buy);

    EXPECT_EQ(book->getOrders().count(sell.id), 0u);
    EXPECT_EQ(book->getOrders().count(buy.id),  0u);
}

TEST(Strategy, Sell_ConsumesMultipleBids) {
    auto book = makeBook();

    auto buy1 = Order("TST", BUY, 3.0, 100.0);
    buy1.timestamp = std::chrono::system_clock::time_point(std::chrono::microseconds(1000));
    auto buy2 = Order("TST", BUY, 4.0, 100.0);
    buy2.timestamp = std::chrono::system_clock::time_point(std::chrono::microseconds(2000));

    book->insertOrder(buy1);
    book->insertOrder(buy2);

    auto sell = Order("TST", SELL, 7.0, 100.0);
    book->insertOrder(sell);

    EXPECT_EQ(book->getOrders().count(buy1.id), 0u);
    EXPECT_EQ(book->getOrders().count(buy2.id), 0u);
    EXPECT_EQ(book->getOrders().count(sell.id), 0u);
}
