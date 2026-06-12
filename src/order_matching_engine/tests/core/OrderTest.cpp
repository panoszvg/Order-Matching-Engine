#include <gtest/gtest.h>
#include "Order.h"
#include <chrono>
#include <set>

TEST(Order, Construction_SetsSecurityField) {
    Order order("BTC", BUY, 10.0, 500.0);
    EXPECT_EQ(order.security, "BTC");
}

TEST(Order, Construction_SetsType_Buy) {
    Order order("BTC", BUY, 10.0, 500.0);
    EXPECT_EQ(order.type, BUY);
}

TEST(Order, Construction_SetsType_Sell) {
    Order order("BTC", SELL, 10.0, 500.0);
    EXPECT_EQ(order.type, SELL);
}

TEST(Order, Construction_SetsQuantity) {
    Order order("BTC", BUY, 7.5, 500.0);
    EXPECT_DOUBLE_EQ(order.quantity, 7.5);
}

TEST(Order, Construction_SetsPrice) {
    Order order("BTC", BUY, 10.0, 123.4);
    EXPECT_DOUBLE_EQ(order.price, 123.4);
}

TEST(Order, Construction_OriginalQuantityMatchesQuantity) {
    Order order("BTC", BUY, 7.5, 500.0);
    EXPECT_DOUBLE_EQ(order.originalQuantity, order.quantity);
}

TEST(Order, Construction_FulfilledDefaultsToNotFulfilled) {
    Order order("BTC", BUY, 10.0, 500.0);
    EXPECT_EQ(order.fulfilled, NOT_FULFILLED);
}

TEST(Order, Construction_IdIsNotEmpty) {
    Order order("BTC", BUY, 10.0, 500.0);
    EXPECT_FALSE(order.id.empty());
}

TEST(Order, Construction_IdHasUUIDFormat) {
    Order order("BTC", BUY, 10.0, 500.0);
    // UUID format: 8-4-4-4-12 hex chars separated by hyphens
    EXPECT_EQ(order.id.size(), 36u);
    EXPECT_EQ(order.id[8],  '-');
    EXPECT_EQ(order.id[13], '-');
    EXPECT_EQ(order.id[18], '-');
    EXPECT_EQ(order.id[23], '-');
}

TEST(Order, Construction_TwoOrdersHaveDifferentIds) {
    Order a("BTC", BUY, 10.0, 500.0);
    Order b("BTC", BUY, 10.0, 500.0);
    EXPECT_NE(a.id, b.id);
}

TEST(Order, Construction_ManyOrdersHaveUniqueIds) {
    std::set<std::string> ids;
    for (int i = 0; i < 1000; ++i) {
        Order o("BTC", BUY, 1.0, 1.0);
        ids.insert(o.id);
    }
    EXPECT_EQ(ids.size(), 1000u);
}

// Known bug: Order constructor does not initialise timestamp to now().
// timestamp defaults to the system_clock epoch (microseconds = 0).
// Fix: add timestamp(std::chrono::system_clock::now()) to the initialiser list.
TEST(Order, Timestamp_DefaultsToEpoch) {
    Order order("BTC", BUY, 10.0, 500.0);
    EXPECT_EQ(order.getMicroTimestamp(), 0);
}
