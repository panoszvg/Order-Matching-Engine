#include <gtest/gtest.h>
#include "TestHelpers.h"
#include <chrono>

// All strategy tests run through Book so the full insert/match pipeline is exercised.

TEST(Strategy, BuyMatchesCheapestSell) {
    auto book = makeBook();
    auto expensive = Order("TST", SELL, 5.0, 102.0);
    auto cheap     = Order("TST", SELL, 5.0, 100.0);
    book->insertOrder(expensive);
    book->insertOrder(cheap);

    auto buy = Order("TST", BUY, 5.0, 102.0);
    book->insertOrder(buy);

    EXPECT_EQ(book->getOrders().at(cheap.id).fulfilled,     FULLY_FULFILLED);
    EXPECT_EQ(book->getOrders().at(expensive.id).fulfilled, NOT_FULFILLED);
    EXPECT_EQ(book->getOrders().at(buy.id).fulfilled,       FULLY_FULFILLED);
}

TEST(Strategy, SellMatchesHighestBid) {
    auto book = makeBook();
    auto low_bid  = Order("TST", BUY, 5.0, 99.0);
    auto high_bid = Order("TST", BUY, 5.0, 101.0);
    book->insertOrder(low_bid);
    book->insertOrder(high_bid);

    auto sell = Order("TST", SELL, 5.0, 99.0);
    book->insertOrder(sell);

    EXPECT_EQ(book->getOrders().at(high_bid.id).fulfilled, FULLY_FULFILLED);
    EXPECT_EQ(book->getOrders().at(low_bid.id).fulfilled,  NOT_FULFILLED);
    EXPECT_EQ(book->getOrders().at(sell.id).fulfilled,     FULLY_FULFILLED);
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

    EXPECT_EQ(book->getOrders().at(buy.id).fulfilled, FULLY_FULFILLED);
    EXPECT_DOUBLE_EQ(book->getOrders().at(buy.id).quantity, 0.0);

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

    EXPECT_EQ(book->getOrders().at(sell1.id).fulfilled, FULLY_FULFILLED);
    EXPECT_EQ(book->getOrders().at(sell2.id).fulfilled, NOT_FULFILLED);
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

    EXPECT_EQ(book->getOrders().at(buy1.id).fulfilled, FULLY_FULFILLED);
    EXPECT_EQ(book->getOrders().at(buy2.id).fulfilled, NOT_FULFILLED);
}

TEST(Strategy, SkipsCancelledRestingOrders) {
    auto book = makeBook();

    auto sell1 = Order("TST", SELL, 5.0, 100.0);
    sell1.timestamp = std::chrono::system_clock::time_point(std::chrono::microseconds(1000));
    auto sell2 = Order("TST", SELL, 5.0, 100.0);
    sell2.timestamp = std::chrono::system_clock::time_point(std::chrono::microseconds(2000));

    book->insertOrder(sell1);
    book->insertOrder(sell2);

    // Cancel sell1 — it would normally match first due to earlier timestamp
    book->cancelOrder(sell1.id);

    auto buy = Order("TST", BUY, 5.0, 100.0);
    book->insertOrder(buy);

    EXPECT_EQ(book->getOrders().at(sell1.id).fulfilled, CANCELLED);
    EXPECT_EQ(book->getOrders().at(sell2.id).fulfilled, FULLY_FULFILLED);
    EXPECT_EQ(book->getOrders().at(buy.id).fulfilled,   FULLY_FULFILLED);
}

TEST(Strategy, BuyPriceExactlyAtAsk_Matches) {
    auto book = makeBook();
    auto sell = Order("TST", SELL, 5.0, 100.0);
    book->insertOrder(sell);

    auto buy = Order("TST", BUY, 5.0, 100.0); // price == ask
    book->insertOrder(buy);

    EXPECT_EQ(book->getOrders().at(sell.id).fulfilled, FULLY_FULFILLED);
    EXPECT_EQ(book->getOrders().at(buy.id).fulfilled,  FULLY_FULFILLED);
}

TEST(Strategy, BuyPriceAboveAsk_MatchesAtAsk) {
    // Trade happens at the maker (resting sell) price, not taker price.
    // Verify via remaining quantities — no trade record exists yet.
    auto book = makeBook();
    auto sell = Order("TST", SELL, 5.0, 100.0);
    book->insertOrder(sell);

    auto buy = Order("TST", BUY, 5.0, 101.0);
    book->insertOrder(buy);

    EXPECT_EQ(book->getOrders().at(sell.id).fulfilled, FULLY_FULFILLED);
    EXPECT_EQ(book->getOrders().at(buy.id).fulfilled,  FULLY_FULFILLED);
    EXPECT_DOUBLE_EQ(book->getOrders().at(sell.id).quantity, 0.0);
    EXPECT_DOUBLE_EQ(book->getOrders().at(buy.id).quantity,  0.0);
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

    EXPECT_EQ(book->getOrders().at(buy1.id).fulfilled, FULLY_FULFILLED);
    EXPECT_EQ(book->getOrders().at(buy2.id).fulfilled, FULLY_FULFILLED);
    EXPECT_EQ(book->getOrders().at(sell.id).fulfilled, FULLY_FULFILLED);
}
