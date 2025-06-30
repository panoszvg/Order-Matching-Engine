#include "Order.h"
#include <iostream>
#include <random>
#include <sstream>
#include <iomanip>

std::shared_ptr<spdlog::logger> logger = spdlog::basic_logger_mt<spdlog::async_factory>("order_logger", "logs/app.log", true);

Order::Order(string security, OrderType type, double quantity, double price)
    : id(generateUUID()), security(std::move(security)), type(type), quantity(quantity), price(price), fulfilled(NOT_FULFILLED) {}

static std::random_device              rd;
static std::mt19937                    gen(rd());
static std::uniform_int_distribution<> dis(0, 15);
static std::uniform_int_distribution<> dis2(8, 11);

string generateUUID() {
	std::stringstream ss;
	int i;
	ss << std::hex;
	for (i = 0; i < 8; i++) ss << dis(gen);
	ss << "-";
	for (i = 0; i < 4; i++) ss << dis(gen);
	ss << "-4";
	for (i = 0; i < 3; i++) ss << dis(gen);
	ss << "-";
	ss << dis2(gen);
	for (i = 0; i < 3; i++) ss << dis(gen);
	ss << "-";
	for (i = 0; i < 12; i++) ss << dis(gen);
	return ss.str();
}

PriceBucket::PriceBucket(double price) : price(price) {}

void Order::print() const {
    logger->info("  Price: {:.6}, Quantity: {:.6}, Fulfilled: {}", price, quantity,
                 (fulfilled == NOT_FULFILLED ? "no" :
                  fulfilled == PARTIALLY_FULFILLED ? "partially" :
                  fulfilled == FULLY_FULFILLED ? "yes" : "cancelled"));
}
