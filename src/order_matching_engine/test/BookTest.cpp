#include <gtest/gtest.h>
#include "Book.h"
#include "strategy/PriceTimePriorityStrategy.h"
#include <vector>

TEST(Book, Instantiation) {
	std::unique_ptr<IOrderMatchingStrategy> strat{ new PriceTimePriorityStrategy() };
	std::unique_ptr<Security> sec{ new Security("ABC", 0.001, 0.05) };
	EXPECT_NO_THROW({Book book(std::move(strat), std::move(sec));});
}

TEST(Book, Insert_Invalid_Quantity_Zero) {
	auto security = std::make_unique<Security>("ABC", 0.01, 0.05);
	auto book = std::make_shared<Book>(
		std::make_unique<PriceTimePriorityStrategy>(),
		std::move(security)
	);

	auto bad = Order("ABC", BUY, 0.0, 1.0);
	EXPECT_THROW(book->insertOrder(bad), std::invalid_argument);
}

TEST(Book, Insert_Invalid_Quantity_Negative) {
	auto security = std::make_unique<Security>("ABC", 0.01, 0.05);
	auto book = std::make_shared<Book>(
		std::make_unique<PriceTimePriorityStrategy>(),
		std::move(security)
	);

	auto bad = Order("ABC", BUY, -5.0, 1.0);
	EXPECT_THROW(book->insertOrder(bad), std::invalid_argument);
}

TEST(Book, Insert_Invalid_Price_Zero) {
	auto security = std::make_unique<Security>("ABC", 0.01, 0.05);
	auto book = std::make_shared<Book>(
		std::make_unique<PriceTimePriorityStrategy>(),
		std::move(security)
	);

	auto bad = Order("ABC", BUY, 10, 0.0);
	EXPECT_THROW(book->insertOrder(bad), std::invalid_argument);
}

TEST(Book, Insert_Invalid_Price_Negative) {
	auto security = std::make_unique<Security>("ABC", 0.01, 0.05);
	auto book = std::make_shared<Book>(
		std::make_unique<PriceTimePriorityStrategy>(),
		std::move(security)
	);

	auto bad = Order("ABC", BUY, 10, -0.4);
	EXPECT_THROW(book->insertOrder(bad), std::invalid_argument);
}

TEST(Book, Insert_Invalid_Tick_Size) {
	auto security = std::make_unique<Security>("ABC", 0.1, 0.5);
	auto book = std::make_shared<Book>(
		std::make_unique<PriceTimePriorityStrategy>(),
		std::move(security)
	);

	auto bad = Order("ABC", BUY, 10, 1.01);
	EXPECT_THROW(book->insertOrder(bad), std::invalid_argument);
}

TEST(Book, Insert_Valid_Buy_Order) {
	auto security = std::make_unique<Security>("XYZ", 0.1, 0.1);
	auto book = std::make_shared<Book>(
		std::make_unique<PriceTimePriorityStrategy>(),
		std::move(security)
	);

	auto order = Order("XYZ", BUY, 7, 1.0);
	book->insertOrder(order);
	EXPECT_EQ(book->getBuyOrders().total_quantity, 7);
}

TEST(Book, Insert_Valid_Sell_Order) {
	auto security = std::make_unique<Security>("XYZ", 0.1, 0.1);
	auto book = std::make_shared<Book>(
		std::make_unique<PriceTimePriorityStrategy>(),
		std::move(security)
	);

	auto order = Order("XYZ", SELL, 6, 1.5);
	book->insertOrder(order);
	EXPECT_EQ(book->getSellOrders().total_quantity, 6);
}