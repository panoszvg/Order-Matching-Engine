#include "Book.h"
#include "Order.h"
#include "Logger.h"
#include "Parser.h"
#include "Security.h"
#include "SecurityProvider.h"
#include "benchmark/Benchmark.h"
#include "strategy/PriceTimePriorityStrategy.h"
#include "tcp/Server.h"
#include "tcp/JsonOrderHandler.h"
#include <boost/asio/signal_set.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

int main(int argc, char* argv[]) {

	logger->info("");
	logger->info("PROGRAM START");
	logger->info("");

	std::unordered_map<string, shared_ptr<Book>> books;
	
	try {
		SecurityProvider provider("input_files/securities.csv");
		std::unordered_map<string, shared_ptr<Security>> securities = provider.loadSecurities();
		auto strategy = std::make_shared<PriceTimePriorityStrategy>();
		for (auto [securityStr, security] : securities) {
			auto book = std::make_shared<Book>(strategy, security);
			books[securityStr] = book;
		}
	} catch(std::exception &e) {
		logger->error("Securities couldn't be read: {}", e.what());
	}

    if (argc > 1) {
        std::string mode = argv[1];
        if (mode == "benchmark") {
			if (books.empty()) {
				std::cerr << "No books available to benchmark.\n";
				return 1;
			}
			if (mode == "benchmark" && argc > 2) {
				std::string type = argv[2];
				auto book = books.begin()->second;
				if (type == "insertcancel") return runInsertCancelBenchmark(book);
				else if (type == "match") return runMatchSimulationBenchmark(book);
				else if (type == "mixed") return runMixedLoadBenchmark(book);
			}
        }
    }

	string line = "";
	vector<string> lines;
	std::ifstream file("input_files/FIX_Messages.csv");

	while (getline(file, line)) {
		lines.push_back(line);
	}
	file.close();

	shared_ptr<Parser> parser = std::make_shared<Parser>();

	for (auto& line : lines) {
		auto newMessage = parser->parse(line);
		auto newOrder = newMessage->makeOrder();
		try {
			books[newOrder->security]->insertOrder(newOrder);
		} catch (const std::invalid_argument& arg) {
			logger->error("Order rejected: {}", arg.what());
		}
	}

	for (auto& [security, book] : books) {
		logger->info("{} Book after ending:", security);
		book->printBuyOrders();
		book->printSellOrders();
	}
	
	boost::asio::io_context io_context;
	
	auto handler = std::make_shared<JsonOrderHandler>(books);
	auto server = std::make_shared<TcpServer>(io_context, 9000, handler);
	server->startAccept();
	
	boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
	signals.async_wait([&](auto, auto) {
		std::cout << "Signal received, stopping.\n";
		io_context.stop();
	});
	
	io_context.run();

	return 0;
}