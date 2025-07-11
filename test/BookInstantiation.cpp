#include <gtest/gtest.h>
#include "../Book.h"
#include "../strategy/PriceTimePriorityStrategy.h"

TEST(Book, TestInstantiation) {
    // build a base-typed unique_ptr directly
    std::unique_ptr<IOrderMatchingStrategy> strat{
        new PriceTimePriorityStrategy()
    };
    // should construct without throwing
    EXPECT_NO_THROW({
        Book book(std::move(strat));
    });
}
