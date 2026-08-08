#include <gtest/gtest.h>
#include "TestHelpers.h"
#include "strategy/TopProRataStrategy.h"
#include <chrono>

// All strategy tests run through Book so the full insert/match pipeline is exercised.
// Fully-filled orders are erased from allOrders: count(id)==0 means filled.
// Unfilled/partial orders are still in allOrders and can be inspected via getOrders().at(id).
//
// TopProRataStrategy: PRIORITY_LOT = 5.0. Tests use lotSize = 1.0 so floored shares
// land on whole numbers and results are easy to hand-verify.

namespace {
    std::shared_ptr<Book> makeProRataBook(double tick = 0.1, double lotSize = 1.0) {
        return makeBookWithStrategy<TopProRataStrategy>("TST", tick, lotSize);
    }

    Order makeTimedOrder(const std::string& symbol, OrderType type, double qty, double price, int64_t micros) {
        Order o(symbol, type, qty, price);
        o.timestamp = std::chrono::system_clock::time_point(std::chrono::microseconds(micros));
        return o;
    }
}

TEST(TopProRataStrategy, SingleOrderAtLevel_DegeneratesToPriceTime) {
    auto book = makeProRataBook();
    auto sell = Order("TST", SELL, 10.0, 100.0);
    book->insertOrder(sell);

    auto buy = Order("TST", BUY, 6.0, 100.0);
    book->insertOrder(buy);

    EXPECT_EQ(book->getOrders().count(buy.id), 0u);  // fully filled -> erased
    EXPECT_EQ(book->getOrders().at(sell.id).fulfilled, PARTIALLY_FULFILLED);
    EXPECT_DOUBLE_EQ(book->getOrders().at(sell.id).quantity, 4.0);
}

TEST(TopProRataStrategy, IncomingBelowPriorityLot_PriorityOrderAbsorbsAll) {
    auto book = makeProRataBook();

    auto sell1 = makeTimedOrder("TST", SELL, 10.0, 100.0, 1000);
    auto sell2 = makeTimedOrder("TST", SELL, 30.0, 100.0, 2000);
    book->insertOrder(sell1);
    book->insertOrder(sell2);

    // Incoming (3.0) < PRIORITY_LOT (5.0): priority order alone absorbs the whole trade.
    auto buy = Order("TST", BUY, 3.0, 100.0);
    book->insertOrder(buy);

    EXPECT_EQ(book->getOrders().count(buy.id), 0u);  // fully filled -> erased
    EXPECT_EQ(book->getOrders().at(sell1.id).fulfilled, PARTIALLY_FULFILLED);
    EXPECT_DOUBLE_EQ(book->getOrders().at(sell1.id).quantity, 7.0);
    EXPECT_EQ(book->getOrders().at(sell2.id).fulfilled, NOT_FULFILLED);
    EXPECT_DOUBLE_EQ(book->getOrders().at(sell2.id).quantity, 30.0);
}

TEST(TopProRataStrategy, IncomingCoversWholeLevel_FullyFillsLikePriceTime) {
    auto book = makeProRataBook();

    auto sell1 = makeTimedOrder("TST", SELL, 10.0, 100.0, 1000);
    auto sell2 = makeTimedOrder("TST", SELL, 30.0, 100.0, 2000);
    auto sell3 = makeTimedOrder("TST", SELL, 60.0, 100.0, 3000);
    book->insertOrder(sell1);
    book->insertOrder(sell2);
    book->insertOrder(sell3);

    // Incoming (100) >= level total (100): whole level fully fills and clears.
    auto buy = Order("TST", BUY, 100.0, 100.0);
    book->insertOrder(buy);

    EXPECT_EQ(book->getOrders().count(buy.id),   0u);
    EXPECT_EQ(book->getOrders().count(sell1.id), 0u);
    EXPECT_EQ(book->getOrders().count(sell2.id), 0u);
    EXPECT_EQ(book->getOrders().count(sell3.id), 0u);
    EXPECT_TRUE(book->getSellOrders().empty());
}

// Resting sells S1=10 (earliest), S2=30, S3=60 at the same price level (total=100).
// Incoming buy=41. PRIORITY_LOT=5, lotSize=1.
//
// Priority slice: S1 fills 5 (S1 -> 5 remaining). Pool = 41 - 5 = 36.
// Pool weights (post-slice sizes): S1=5, S2=30, S3=60, total=95.
// Raw shares: S1=1.89, S2=11.37, S3=22.74 -> floored: S1=1, S2=11, S3=22 (sum=34).
// Remainder = 36 - 34 = 2 lots, round-robin in time order: S1 +1, S2 +1.
// Pool fills: S1=2, S2=12, S3=22.
// Final fills: S1 = 5(priority) + 2(pool) = 7, S2 = 12, S3 = 22. Total = 41.
// Survivors: S1=3, S2=18, S3=38 (total remaining = 59, matching price-time's leftover).
TEST(TopProRataStrategy, DesignWorkedExample_PrioritySlicePlusProRataPoolWithRemainder) {
    auto book = makeProRataBook();

    auto s1 = makeTimedOrder("TST", SELL, 10.0, 100.0, 1000);
    auto s2 = makeTimedOrder("TST", SELL, 30.0, 100.0, 2000);
    auto s3 = makeTimedOrder("TST", SELL, 60.0, 100.0, 3000);
    book->insertOrder(s1);
    book->insertOrder(s2);
    book->insertOrder(s3);

    auto buy = Order("TST", BUY, 41.0, 100.0);
    book->insertOrder(buy);

    EXPECT_EQ(book->getOrders().count(buy.id), 0u);  // fully filled -> erased

    ASSERT_EQ(book->getOrders().count(s1.id), 1u);
    ASSERT_EQ(book->getOrders().count(s2.id), 1u);
    ASSERT_EQ(book->getOrders().count(s3.id), 1u);

    EXPECT_DOUBLE_EQ(book->getOrders().at(s1.id).quantity, 3.0);
    EXPECT_DOUBLE_EQ(book->getOrders().at(s2.id).quantity, 18.0);
    EXPECT_DOUBLE_EQ(book->getOrders().at(s3.id).quantity, 38.0);

    EXPECT_EQ(book->getOrders().at(s1.id).fulfilled, PARTIALLY_FULFILLED);
    EXPECT_EQ(book->getOrders().at(s2.id).fulfilled, PARTIALLY_FULFILLED);
    EXPECT_EQ(book->getOrders().at(s3.id).fulfilled, PARTIALLY_FULFILLED);
}

TEST(TopProRataStrategy, BiggerRestingOrderGetsBiggerProRataShare) {
    auto book = makeProRataBook();

    // Two orders beyond the priority slot, sized 1:3 -> pool shares should follow that ratio.
    auto small = makeTimedOrder("TST", SELL, 10.0, 100.0, 1000);  // priority order
    auto medium = makeTimedOrder("TST", SELL, 20.0, 100.0, 2000);
    auto large  = makeTimedOrder("TST", SELL, 60.0, 100.0, 3000);
    book->insertOrder(small);
    book->insertOrder(medium);
    book->insertOrder(large);

    auto buy = Order("TST", BUY, 45.0, 100.0);
    book->insertOrder(buy);

    ASSERT_EQ(book->getOrders().count(medium.id), 1u);
    ASSERT_EQ(book->getOrders().count(large.id),  1u);

    double mediumFilled = 20.0 - book->getOrders().at(medium.id).quantity;
    double largeFilled  = 60.0 - book->getOrders().at(large.id).quantity;

    EXPECT_GT(largeFilled, mediumFilled);
}

TEST(TopProRataStrategy, SweepsAcrossPriceLevelsWhenLevelExhausted) {
    auto book = makeProRataBook();

    // Best level (100.0) has only 5 total -> fully consumed by priority slice.
    auto best = Order("TST", SELL, 5.0, 100.0);
    book->insertOrder(best);

    // Next level (101.0) should still be reached by the sweep.
    auto next = Order("TST", SELL, 20.0, 101.0);
    book->insertOrder(next);

    auto buy = Order("TST", BUY, 15.0, 101.0);
    book->insertOrder(buy);

    EXPECT_EQ(book->getOrders().count(buy.id),  0u);   // fully filled -> erased
    EXPECT_EQ(book->getOrders().count(best.id), 0u);   // fully filled at first level -> erased
    ASSERT_EQ(book->getOrders().count(next.id), 1u);
    EXPECT_DOUBLE_EQ(book->getOrders().at(next.id).quantity, 10.0);
}

// Three price levels: 100.0 (S1=10, S2=30, S3=60; total=100), 101.0 (S4=8, S5=12;
// total=20), 102.0 (S6=5, S7=5; total=10).
//
// First BUY=100 @ 100.0 is sized to exactly exhaust level 100.0, so it fills via
// the "level total <= incoming" branch (same as price-time) and clears it entirely,
// leaving 101.0/102.0 untouched.
//
// Second BUY=25 @ 102.0: level 100.0 is gone, so the sweep must reach 101.0.
// levelQuantity(101.0)=20 <= 25 -> fully fills and clears 101.0 via the same
// "fill all" branch, carrying 5 remaining into 102.0. There levelQuantity(102.0)=10
// > 5, so it hits the pro-rata branch: the priority slice alone (S6, earliest,
// qty=5, min(PRIORITY_LOT=5, min(5,5))=5) fully absorbs the remaining 5.
// Final state: only S7=5 left resting at 102.0.
TEST(TopProRataStrategy, SweepsAcrossMultipleLevelsWithPriorityAndPoolFills) {
    auto book = makeProRataBook();

    auto s1 = makeTimedOrder("TST", SELL, 10.0, 100.0, 1000);
    auto s2 = makeTimedOrder("TST", SELL, 30.0, 100.0, 2000);
    auto s3 = makeTimedOrder("TST", SELL, 60.0, 100.0, 3000);
    auto s4 = makeTimedOrder("TST", SELL, 8.0,  101.0, 4000);
    auto s5 = makeTimedOrder("TST", SELL, 12.0, 101.0, 5000);
    auto s6 = makeTimedOrder("TST", SELL, 5.0,  102.0, 6000);
    auto s7 = makeTimedOrder("TST", SELL, 5.0,  102.0, 7000);
    book->insertOrder(s1);
    book->insertOrder(s2);
    book->insertOrder(s3);
    book->insertOrder(s4);
    book->insertOrder(s5);
    book->insertOrder(s6);
    book->insertOrder(s7);

    auto firstBuy = Order("TST", BUY, 100.0, 100.0);
    book->insertOrder(firstBuy);

    EXPECT_EQ(book->getOrders().count(firstBuy.id), 0u);  // fully filled -> erased
    EXPECT_EQ(book->getOrders().count(s1.id), 0u);
    EXPECT_EQ(book->getOrders().count(s2.id), 0u);
    EXPECT_EQ(book->getOrders().count(s3.id), 0u);

    ASSERT_EQ(book->getOrders().count(s4.id), 1u);
    ASSERT_EQ(book->getOrders().count(s5.id), 1u);
    ASSERT_EQ(book->getOrders().count(s6.id), 1u);
    ASSERT_EQ(book->getOrders().count(s7.id), 1u);
    EXPECT_DOUBLE_EQ(book->getOrders().at(s4.id).quantity, 8.0);
    EXPECT_DOUBLE_EQ(book->getOrders().at(s5.id).quantity, 12.0);
    EXPECT_DOUBLE_EQ(book->getOrders().at(s6.id).quantity, 5.0);
    EXPECT_DOUBLE_EQ(book->getOrders().at(s7.id).quantity, 5.0);

    auto secondBuy = Order("TST", BUY, 25.0, 102.0);
    book->insertOrder(secondBuy);

    EXPECT_EQ(book->getOrders().count(secondBuy.id), 0u);  // fully filled -> erased
    EXPECT_EQ(book->getOrders().count(s4.id), 0u);
    EXPECT_EQ(book->getOrders().count(s5.id), 0u);
    EXPECT_EQ(book->getOrders().count(s6.id), 0u);

    ASSERT_EQ(book->getOrders().count(s7.id), 1u);
    EXPECT_DOUBLE_EQ(book->getOrders().at(s7.id).quantity, 5.0);
    EXPECT_EQ(book->getOrders().at(s7.id).fulfilled, NOT_FULFILLED);
}

TEST(TopProRataStrategy, NoCross_NoMatch) {
    auto book = makeProRataBook();
    auto sell = Order("TST", SELL, 5.0, 101.0);
    auto buy  = Order("TST", BUY,  5.0, 100.0);
    book->insertOrder(sell);
    book->insertOrder(buy);

    EXPECT_EQ(book->getOrders().at(sell.id).fulfilled, NOT_FULFILLED);
    EXPECT_EQ(book->getOrders().at(buy.id).fulfilled,  NOT_FULFILLED);
}

TEST(TopProRataStrategy, SkipsCancelledRestingOrders) {
    auto book = makeProRataBook();

    auto sell1 = makeTimedOrder("TST", SELL, 5.0, 100.0, 1000);
    auto sell2 = makeTimedOrder("TST", SELL, 5.0, 100.0, 2000);
    book->insertOrder(sell1);
    book->insertOrder(sell2);

    book->cancelOrder(sell1.id);

    auto buy = Order("TST", BUY, 5.0, 100.0);
    book->insertOrder(buy);

    EXPECT_EQ(book->getOrders().count(sell1.id), 0u);
    EXPECT_EQ(book->getOrders().count(sell2.id), 0u);
    EXPECT_EQ(book->getOrders().count(buy.id),   0u);
    EXPECT_TRUE(book->getSellOrders().empty());
    EXPECT_TRUE(book->getBuyOrders().empty());
}
