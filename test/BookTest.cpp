#include <gtest/gtest.h>
#include "../Book.h"
#include "../strategy/PriceTimePriorityStrategy.h"
#include <vector>

TEST(Book, Instantiation) {
	std::unique_ptr<IOrderMatchingStrategy> strat{
		new PriceTimePriorityStrategy()
	};
	EXPECT_NO_THROW({Book book(std::move(strat));});
}

TEST(Book, Insert_Invalid_Quantity_Zero) {
	auto book = std::make_shared<Book>(std::make_unique<PriceTimePriorityStrategy>());
	std::vector<std::shared_ptr<Security>> securities;
	securities.push_back(std::make_shared<Security>("ABC", 0.01, 0.05));

	book->addSecurities(securities);
	auto bad = std::make_shared<Order>("ABC", BUY, 0.0, 1.0);
	EXPECT_THROW(book->insertOrder(bad), std::invalid_argument);
}

TEST(Book, Insert_Invalid_Quantity_Negative) {
	auto book = std::make_shared<Book>(std::make_unique<PriceTimePriorityStrategy>());
	std::vector<std::shared_ptr<Security>> securities;
	securities.push_back(std::make_shared<Security>("ABC", 0.01, 0.05));

	book->addSecurities(securities);
	auto bad = std::make_shared<Order>("ABC", BUY, -5.0, 1.0);
	EXPECT_THROW(book->insertOrder(bad), std::invalid_argument);
}

TEST(Book, Insert_Invalid_Price_Zero) {
	auto book = std::make_shared<Book>(std::make_unique<PriceTimePriorityStrategy>());
	std::vector<std::shared_ptr<Security>> securities;
	securities.push_back(std::make_shared<Security>("ABC", 0.01, 0.05));

	book->addSecurities(securities);
	auto bad = std::make_shared<Order>("ABC", BUY, 10, 0.0);
	EXPECT_THROW(book->insertOrder(bad), std::invalid_argument);
}

TEST(Book, Insert_Invalid_Price_Negative) {
	auto book = std::make_shared<Book>(std::make_unique<PriceTimePriorityStrategy>());
	std::vector<std::shared_ptr<Security>> securities;
	securities.push_back(std::make_shared<Security>("ABC", 0.01, 0.05));

	book->addSecurities(securities);
	auto bad = std::make_shared<Order>("ABC", BUY, 10, -0.4);
	EXPECT_THROW(book->insertOrder(bad), std::invalid_argument);
}

TEST(Book, Insert_Invalid_Tick_Size) {
	auto book = std::make_shared<Book>(std::make_unique<PriceTimePriorityStrategy>());
	std::vector<std::shared_ptr<Security>> securities;
	securities.push_back(std::make_shared<Security>("ABC", 0.1, 0.5));
	book->addSecurities(securities);
	auto bad = std::make_shared<Order>("ABC", BUY, 10, 1.01);
	EXPECT_THROW(book->insertOrder(bad), std::invalid_argument);
}

TEST(Book, Insert_Valid_Buy_Order) {
	auto book = std::make_shared<Book>(std::make_unique<PriceTimePriorityStrategy>());
	std::vector<std::shared_ptr<Security>> securities;
	securities.push_back(std::make_shared<Security>("XYZ", 0.1, 0.1));
	book->addSecurities(securities);
	auto order = std::make_shared<Order>("XYZ", BUY, 7, 1.0);
	book->insertOrder(order);
	auto &buyBucket = book->getBuyOrders().at("XYZ").at(1.0);
	EXPECT_EQ(buyBucket->total_quantity, 7);
}

TEST(Book, Insert_Valid_Sell_Order) {
	auto book = std::make_shared<Book>(std::make_unique<PriceTimePriorityStrategy>());
	std::vector<std::shared_ptr<Security>> securities;
	securities.push_back(std::make_shared<Security>("XYZ", 0.1, 0.1));
	book->addSecurities(securities);
	auto order = std::make_shared<Order>("XYZ", SELL, 6, 1.5);
	book->insertOrder(order);
	auto &sellBucket = book->getSellOrders().at("XYZ").at(1.5);
	EXPECT_EQ(sellBucket->total_quantity, 6);
}