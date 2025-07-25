#include <chrono>
#include <iostream>
#include <memory>
#include "../Order.h"
#include "../Book.h"

using Clock = std::chrono::high_resolution_clock;

std::shared_ptr<Order> createTestOrder(double price, double quantity, const std::string& security = "TEST", const std::string& typeStr = "BUY") {
	OrderType type = (typeStr == "BUY") ? BUY : SELL;
	return std::make_shared<Order>(security, type, price, quantity);
}

int runInsertCancelBenchmark(std::shared_ptr<Book> book) {
	std::cout << "[Insert + Cancel Benchmark]\n";

	std::vector<std::shared_ptr<Order>> insertedOrders;
	auto start = Clock::now();

	for (int i = 0; i < 1000000; ++i) {
		auto order = createTestOrder(100.0 + (i % 10), 1.0, book->getSecurity()->getSymbol(), "BUY");
		insertedOrders.push_back(order);
		book->insertOrder(order);
	}

	for (int i = 0; i < 100000; ++i) {
		book->cancelOrder(insertedOrders[i]->id);
	}

	auto end = Clock::now();
	std::chrono::duration<double> elapsed = end - start;
	double throughput = insertedOrders.size() / elapsed.count();
	std::cout << "Inserted: " << insertedOrders.size() 
		<< " | Cancelled: " << 100000 
		<< " | Duration: " << elapsed.count() << "s"
		<< " | Throughput: " << throughput << " orders/sec" << std::endl;

	return 0;
}

int runMatchSimulationBenchmark(std::shared_ptr<Book> book) {
	std::cout << "[Match Simulation Benchmark]\n";

	auto start = Clock::now();

	int orderCount = 0;
	for (int i = 0; i < 500000; ++i) {
		auto buy = createTestOrder(101.0, 1.0, book->getSecurity()->getSymbol(), "BUY");
		auto sell = createTestOrder(100.0, 1.0, book->getSecurity()->getSymbol(), "SELL");
		book->insertOrder(buy);
		book->insertOrder(sell);
		orderCount += 2;
	}

	auto end = Clock::now();
	std::chrono::duration<double> elapsed = end - start;
	double throughput = orderCount / elapsed.count();
	std::cout << "Orders inserted: " << orderCount
			<< " | Duration: " << elapsed.count() << "s"
			<< " | Throughput: " << throughput << " orders/sec\n";
	return 0;
}

int runMixedLoadBenchmark(std::shared_ptr<Book> book) {
	std::cout << "[Mixed Load Benchmark]\n";

	std::vector<std::shared_ptr<Order>> insertedOrders;
	auto start = Clock::now();

	int cancelCount = 0;
	for (int i = 0; i < 1000000; ++i) {
		auto type = (i % 2 == 0) ? "BUY" : "SELL";
		auto order = createTestOrder(100.0 + (i % 5), 1.0, book->getSecurity()->getSymbol(), type);
		insertedOrders.push_back(order);
		book->insertOrder(order);

		if (i % 20 == 0 && i > 0) {
			book->cancelOrder(insertedOrders[i - 20]->id); // occasional cancel
			++cancelCount;
		}
	}

	auto end = Clock::now();
	std::chrono::duration<double> elapsed = end - start;
	double throughput = insertedOrders.size() / elapsed.count();
	std::cout << "Orders inserted: " << insertedOrders.size()
			<< " | Cancelled: " << cancelCount
			<< " | Duration: " << elapsed.count() << "s"
			<< " | Throughput: " << throughput << " orders/sec\n";
	return 0;
}
