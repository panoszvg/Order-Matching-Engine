#include <gtest/gtest.h>
#include "TestHelpers.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// ── Validation ────────────────────────────────────────────────────────────────

TEST(Book, Instantiation) {
    EXPECT_NO_THROW(makeBook());
}

TEST(Book, Insert_Invalid_Quantity_Zero) {
    auto book = makeBook();
    auto order = Order("TST", BUY, 0.0, 1.0);
    EXPECT_THROW(book->insertOrder(order), std::invalid_argument);
}

TEST(Book, Insert_Invalid_Quantity_Negative) {
    auto book = makeBook();
    auto order = Order("TST", BUY, -5.0, 1.0);
    EXPECT_THROW(book->insertOrder(order), std::invalid_argument);
}

TEST(Book, Insert_Invalid_Price_Zero) {
    auto book = makeBook();
    auto order = Order("TST", BUY, 10.0, 0.0);
    EXPECT_THROW(book->insertOrder(order), std::invalid_argument);
}

TEST(Book, Insert_Invalid_Price_Negative) {
    auto book = makeBook();
    auto order = Order("TST", BUY, 10.0, -1.0);
    EXPECT_THROW(book->insertOrder(order), std::invalid_argument);
}

TEST(Book, Insert_Invalid_Tick_Size) {
    auto book = makeBook("TST", 0.1, 0.5);
    auto order = Order("TST", BUY, 10.0, 1.05); // 1.05 not aligned to 0.1
    EXPECT_THROW(book->insertOrder(order), std::invalid_argument);
}

TEST(Book, Insert_Valid_Buy_Order) {
    auto book = makeBook();
    auto order = Order("TST", BUY, 7.0, 1.0);
    EXPECT_NO_THROW(book->insertOrder(order));
    EXPECT_DOUBLE_EQ(book->getBuyOrders().total_quantity, 7.0);
}

TEST(Book, Insert_Valid_Sell_Order) {
    auto book = makeBook();
    auto order = Order("TST", SELL, 6.0, 1.5);
    EXPECT_NO_THROW(book->insertOrder(order));
    EXPECT_DOUBLE_EQ(book->getSellOrders().total_quantity, 6.0);
}

TEST(Book, Insert_Duplicate_Id_Throws) {
    auto book = makeBook();
    auto order = Order("TST", BUY, 5.0, 1.0);
    book->insertOrder(order);
    EXPECT_THROW(book->insertOrder(order), std::invalid_argument);
}

// ── Matching ─────────────────────────────────────────────────────────────────

TEST(Book, Match_Full_Buy_Against_Resting_Sell) {
    auto book = makeBook();
    auto sell = Order("TST", SELL, 5.0, 100.0);
    auto buy  = Order("TST", BUY,  5.0, 100.0);
    book->insertOrder(sell);
    book->insertOrder(buy);

    EXPECT_EQ(book->getOrders().at(sell.id).fulfilled, FULLY_FULFILLED);
    EXPECT_EQ(book->getOrders().at(buy.id).fulfilled,  FULLY_FULFILLED);
    EXPECT_TRUE(book->getSellOrders().queue.empty());
    EXPECT_TRUE(book->getBuyOrders().queue.empty());
}

TEST(Book, Match_Full_Sell_Against_Resting_Buy) {
    auto book = makeBook();
    auto buy  = Order("TST", BUY,  5.0, 100.0);
    auto sell = Order("TST", SELL, 5.0, 100.0);
    book->insertOrder(buy);
    book->insertOrder(sell);

    EXPECT_EQ(book->getOrders().at(buy.id).fulfilled,  FULLY_FULFILLED);
    EXPECT_EQ(book->getOrders().at(sell.id).fulfilled, FULLY_FULFILLED);
}

TEST(Book, NoMatch_Buy_Price_Below_Ask) {
    auto book = makeBook();
    auto sell = Order("TST", SELL, 5.0, 101.0);
    auto buy  = Order("TST", BUY,  5.0, 100.0);
    book->insertOrder(sell);
    book->insertOrder(buy);

    EXPECT_EQ(book->getOrders().at(sell.id).fulfilled, NOT_FULFILLED);
    EXPECT_EQ(book->getOrders().at(buy.id).fulfilled,  NOT_FULFILLED);
    EXPECT_EQ(book->getSellOrders().total_quantity, 5.0);
    EXPECT_EQ(book->getBuyOrders().total_quantity,  5.0);
}

TEST(Book, NoMatch_Sell_Price_Above_Bid) {
    auto book = makeBook();
    auto buy  = Order("TST", BUY,  5.0, 100.0);
    auto sell = Order("TST", SELL, 5.0, 101.0);
    book->insertOrder(buy);
    book->insertOrder(sell);

    EXPECT_EQ(book->getOrders().at(buy.id).fulfilled,  NOT_FULFILLED);
    EXPECT_EQ(book->getOrders().at(sell.id).fulfilled, NOT_FULFILLED);
}

TEST(Book, PartialFill_Buy_Less_Than_Sell) {
    auto book = makeBook();
    auto sell = Order("TST", SELL, 10.0, 100.0);
    auto buy  = Order("TST", BUY,   3.0, 100.0);
    book->insertOrder(sell);
    book->insertOrder(buy);

    EXPECT_EQ(book->getOrders().at(buy.id).fulfilled,  FULLY_FULFILLED);
    EXPECT_EQ(book->getOrders().at(sell.id).fulfilled, PARTIALLY_FULFILLED);
    EXPECT_DOUBLE_EQ(book->getOrders().at(sell.id).quantity, 7.0);
    EXPECT_DOUBLE_EQ(book->getSellOrders().total_quantity, 7.0);
}

TEST(Book, PartialFill_Buy_More_Than_Sell) {
    auto book = makeBook();
    auto sell = Order("TST", SELL,  3.0, 100.0);
    auto buy  = Order("TST", BUY,  10.0, 100.0);
    book->insertOrder(sell);
    book->insertOrder(buy);

    EXPECT_EQ(book->getOrders().at(sell.id).fulfilled, FULLY_FULFILLED);
    EXPECT_EQ(book->getOrders().at(buy.id).fulfilled,  PARTIALLY_FULFILLED);
    EXPECT_DOUBLE_EQ(book->getOrders().at(buy.id).quantity, 7.0);
    EXPECT_DOUBLE_EQ(book->getBuyOrders().total_quantity, 7.0);
}

TEST(Book, Match_Buy_Consumes_Multiple_Sells) {
    auto book = makeBook();

    auto sell1 = Order("TST", SELL, 3.0, 100.0);
    sell1.timestamp = std::chrono::system_clock::time_point(std::chrono::microseconds(1000));
    auto sell2 = Order("TST", SELL, 4.0, 100.0);
    sell2.timestamp = std::chrono::system_clock::time_point(std::chrono::microseconds(2000));

    book->insertOrder(sell1);
    book->insertOrder(sell2);

    auto buy = Order("TST", BUY, 7.0, 100.0);
    book->insertOrder(buy);

    EXPECT_EQ(book->getOrders().at(sell1.id).fulfilled, FULLY_FULFILLED);
    EXPECT_EQ(book->getOrders().at(sell2.id).fulfilled, FULLY_FULFILLED);
    EXPECT_EQ(book->getOrders().at(buy.id).fulfilled,   FULLY_FULFILLED);
}

TEST(Book, Match_Price_Priority_CheapestSellFirst) {
    auto book = makeBook();
    auto expensive = Order("TST", SELL, 5.0, 101.0);
    auto cheap     = Order("TST", SELL, 5.0, 100.0);
    book->insertOrder(expensive);
    book->insertOrder(cheap);

    auto buy = Order("TST", BUY, 5.0, 102.0);
    book->insertOrder(buy);

    EXPECT_EQ(book->getOrders().at(cheap.id).fulfilled,     FULLY_FULFILLED);
    EXPECT_EQ(book->getOrders().at(expensive.id).fulfilled, NOT_FULFILLED);
    EXPECT_EQ(book->getOrders().at(buy.id).fulfilled,       FULLY_FULFILLED);
}

TEST(Book, Match_Price_Priority_HighestBidFirst) {
    auto book = makeBook();
    auto low_bid  = Order("TST", BUY, 5.0, 100.0);
    auto high_bid = Order("TST", BUY, 5.0, 101.0);
    book->insertOrder(low_bid);
    book->insertOrder(high_bid);

    auto sell = Order("TST", SELL, 5.0, 100.0);
    book->insertOrder(sell);

    EXPECT_EQ(book->getOrders().at(high_bid.id).fulfilled, FULLY_FULFILLED);
    EXPECT_EQ(book->getOrders().at(low_bid.id).fulfilled,  NOT_FULFILLED);
    EXPECT_EQ(book->getOrders().at(sell.id).fulfilled,     FULLY_FULFILLED);
}

// ── Cancellation ─────────────────────────────────────────────────────────────

TEST(Book, Cancel_MarksOrderCancelled) {
    auto book = makeBook();
    auto order = Order("TST", BUY, 5.0, 100.0);
    book->insertOrder(order);
    book->cancelOrder(order.id);

    EXPECT_EQ(book->orderLookup(order.id).fulfilled, CANCELLED);
}

TEST(Book, Cancel_ReducesTotalQuantity) {
    auto book = makeBook();
    auto order = Order("TST", BUY, 5.0, 100.0);
    book->insertOrder(order);
    EXPECT_DOUBLE_EQ(book->getBuyOrders().total_quantity, 5.0);

    book->cancelOrder(order.id);
    EXPECT_DOUBLE_EQ(book->getBuyOrders().total_quantity, 0.0);
}

TEST(Book, Cancel_NonExistent_Throws) {
    auto book = makeBook();
    EXPECT_THROW(book->cancelOrder("does-not-exist"), std::invalid_argument);
}

TEST(Book, Cancel_AlreadyCancelled_IsNoOp) {
    auto book = makeBook();
    auto order = Order("TST", BUY, 5.0, 100.0);
    book->insertOrder(order);
    book->cancelOrder(order.id);
    EXPECT_NO_THROW(book->cancelOrder(order.id));
}

TEST(Book, Cancel_FullyFulfilled_IsNoOp) {
    auto book = makeBook();
    auto sell = Order("TST", SELL, 5.0, 100.0);
    auto buy  = Order("TST", BUY,  5.0, 100.0);
    book->insertOrder(sell);
    book->insertOrder(buy);

    EXPECT_EQ(book->getOrders().at(buy.id).fulfilled, FULLY_FULFILLED);
    EXPECT_NO_THROW(book->cancelOrder(buy.id));
    EXPECT_EQ(book->getOrders().at(buy.id).fulfilled, FULLY_FULFILLED);
}

// ── Modification ─────────────────────────────────────────────────────────────

TEST(Book, Modify_UpdatesRestingOrder) {
    auto book = makeBook();
    auto sell = Order("TST", SELL, 10.0, 100.0);
    book->insertOrder(sell);

    EXPECT_NO_THROW(book->modifyOrder(sell.id, 10.0, 101.0));

    // Original order should be cancelled
    EXPECT_EQ(book->orderLookup(sell.id).fulfilled, CANCELLED);
    // A new sell with the updated price should now hold the book quantity
    EXPECT_NEAR(book->getSellOrders().total_quantity, 10.0, 1e-9);
}

TEST(Book, Modify_NonExistent_Throws) {
    auto book = makeBook();
    EXPECT_THROW(book->modifyOrder("does-not-exist", 5.0, 100.0), std::out_of_range);
}

TEST(Book, Modify_NewQtyBelowFilledQty_Throws) {
    auto book = makeBook();
    auto sell = Order("TST", SELL, 10.0, 100.0);
    book->insertOrder(sell);

    // Partially fill: buy 3 units
    auto buy = Order("TST", BUY, 3.0, 100.0);
    book->insertOrder(buy);
    // sell has 7 remaining, 3 filled — newQty=2 is below filledQty=3
    EXPECT_THROW(book->modifyOrder(sell.id, 2.0, 100.0), std::invalid_argument);
}

// ── Snapshot (toJson) ─────────────────────────────────────────────────────────

TEST(Book, ToJson_EmptyBook) {
    auto book = makeBook("BTC");
    json j = book->toJson();

    EXPECT_EQ(j["symbol"], "BTC");
    EXPECT_TRUE(j["bids"].empty());
    EXPECT_TRUE(j["asks"].empty());
}

TEST(Book, ToJson_ActiveBuyIncluded) {
    auto book = makeBook();
    auto order = Order("TST", BUY, 5.0, 100.0);
    book->insertOrder(order);
    json j = book->toJson();

    ASSERT_EQ(j["bids"].size(), 1u);
    EXPECT_DOUBLE_EQ(j["bids"][0]["price"].get<double>(),    100.0);
    EXPECT_DOUBLE_EQ(j["bids"][0]["quantity"].get<double>(), 5.0);
    EXPECT_TRUE(j["asks"].empty());
}

TEST(Book, ToJson_ActiveSellIncluded) {
    auto book = makeBook();
    auto order = Order("TST", SELL, 3.0, 101.0);
    book->insertOrder(order);
    json j = book->toJson();

    ASSERT_EQ(j["asks"].size(), 1u);
    EXPECT_DOUBLE_EQ(j["asks"][0]["price"].get<double>(),    101.0);
    EXPECT_DOUBLE_EQ(j["asks"][0]["quantity"].get<double>(), 3.0);
    EXPECT_TRUE(j["bids"].empty());
}

TEST(Book, ToJson_CancelledOrderExcluded) {
    auto book = makeBook();
    auto order = Order("TST", BUY, 5.0, 100.0);
    book->insertOrder(order);
    book->cancelOrder(order.id);
    json j = book->toJson();

    EXPECT_TRUE(j["bids"].empty());
}

TEST(Book, ToJson_FullyFilledOrdersExcluded) {
    auto book = makeBook();
    auto sell = Order("TST", SELL, 5.0, 100.0);
    auto buy  = Order("TST", BUY,  5.0, 100.0);
    book->insertOrder(sell);
    book->insertOrder(buy);
    json j = book->toJson();

    EXPECT_TRUE(j["bids"].empty());
    EXPECT_TRUE(j["asks"].empty());
}

TEST(Book, ToJson_PartiallyFilledOrderIncluded) {
    auto book = makeBook();
    auto sell = Order("TST", SELL, 10.0, 100.0);
    book->insertOrder(sell);

    auto buy = Order("TST", BUY, 3.0, 100.0);
    book->insertOrder(buy);
    // sell partially filled: 7 remaining
    json j = book->toJson();

    EXPECT_TRUE(j["bids"].empty());
    ASSERT_EQ(j["asks"].size(), 1u);
    EXPECT_NEAR(j["asks"][0]["quantity"].get<double>(), 7.0, 1e-9);
}

// ── Total quantity tracking ───────────────────────────────────────────────────

TEST(Book, TotalQuantity_IncreasesOnInsert) {
    auto book = makeBook();
    auto b1 = Order("TST", BUY, 3.0, 100.0);
    auto b2 = Order("TST", BUY, 7.0, 100.0);
    book->insertOrder(b1);
    EXPECT_DOUBLE_EQ(book->getBuyOrders().total_quantity, 3.0);
    book->insertOrder(b2);
    EXPECT_DOUBLE_EQ(book->getBuyOrders().total_quantity, 10.0);
}

TEST(Book, TotalQuantity_DecreasesOnMatch) {
    auto book = makeBook();
    auto sell = Order("TST", SELL, 10.0, 100.0);
    book->insertOrder(sell);
    EXPECT_DOUBLE_EQ(book->getSellOrders().total_quantity, 10.0);

    auto buy = Order("TST", BUY, 4.0, 100.0);
    book->insertOrder(buy);
    EXPECT_DOUBLE_EQ(book->getSellOrders().total_quantity, 6.0);
}

TEST(Book, TotalQuantity_DecreasesOnCancel) {
    auto book = makeBook();
    auto order = Order("TST", SELL, 5.0, 100.0);
    book->insertOrder(order);
    book->cancelOrder(order.id);
    EXPECT_DOUBLE_EQ(book->getSellOrders().total_quantity, 0.0);
}
