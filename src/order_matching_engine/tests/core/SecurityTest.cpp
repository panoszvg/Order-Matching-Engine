#include <gtest/gtest.h>
#include "Security.h"

TEST(Security, GetSymbol) {
    Security sec("ETH", 0.01, 0.1);
    EXPECT_EQ(sec.getSymbol(), "ETH");
}

TEST(Security, GetTickSize) {
    Security sec("ETH", 0.01, 0.1);
    EXPECT_DOUBLE_EQ(sec.getTickSize(), 0.01);
}

TEST(Security, GetBucketSize) {
    Security sec("ETH", 0.01, 0.1);
    EXPECT_DOUBLE_EQ(sec.getBucketSize(), 0.1);
}

TEST(Security, SymbolIsCaseSensitive) {
    Security lower("ada", 0.001, 0.05);
    Security upper("ADA", 0.001, 0.05);
    EXPECT_NE(lower.getSymbol(), upper.getSymbol());
}

TEST(Security, ZeroTickSizeThrows) {
    EXPECT_THROW(Security("ETH", 0.0, 0.1), std::invalid_argument);
}

TEST(Security, NegativeTickSizeThrows) {
    EXPECT_THROW(Security("ETH", -0.01, 0.1), std::invalid_argument);
}

TEST(Security, ZeroBucketSizeThrows) {
    EXPECT_THROW(Security("ETH", 0.01, 0.0), std::invalid_argument);
}

TEST(Security, NegativeBucketSizeThrows) {
    EXPECT_THROW(Security("ETH", 0.01, -0.1), std::invalid_argument);
}
