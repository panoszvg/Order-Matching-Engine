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
