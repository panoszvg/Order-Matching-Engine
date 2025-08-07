#include "AdminCommandHandler.h"

AdminCommandHandler::AdminCommandHandler(std::unordered_map<std::string, std::shared_ptr<Book>>& books)
	: books_(books) {}

void AdminCommandHandler::handle(const std::string& rawMessage, TcpSession& session) {
	try {
		auto json = nlohmann::json::parse(rawMessage);
		std::string type = json["type"];

		if (type == "GET_SNAPSHOT") {
			try {
				std::string symbol = json["symbol"];
				auto book = books_[symbol];
				if (book) {
					nlohmann::json bookJson = book->toJson();

					nlohmann::json response = {
						{"status", "ok"},
						{"message", "Exported snapshot for " + symbol},
						{"symbol", symbol},
						{"book", bookJson}
					};

					session.send(response.dump());
					logger->info("Book was exported and returned");
				} else {
					session.send(R"({"status":"error","message":"Unknown symbol"})");
				}
			} catch (const std::exception& e) {
				session.send(std::string(R"({"status":"error","message":"Exception: )") + e.what() + R"("})");
				logger->error("Failed export book: {}", e.what());
			}
		}
		else if (type == "EXPORT_SNAPSHOT") {
			try {
				std::string symbol = json["symbol"];
				auto book = books_[symbol];
				if (book) {
					book->exportSnapshot();
					session.send(R"({"status":"ok","message":"Exported snapshot for )" + symbol + R"("})");
					logger->info("Book was exported");
				} else {
					session.send(R"({"status":"error","message":"Unknown symbol"})");
				}
			} catch (const std::exception& e) {
				session.send(std::string(R"({"status":"error","message":"Exception: )") + e.what() + R"("})");
				logger->error("Failed export book: {}", e.what());
			}
		} else {
			session.send(R"({"status":"error","message":"Unknown command"})");
		}

	} catch (const std::exception& e) {
		session.send(std::string(R"({"status":"error","message":"Invalid JSON or command: )") + e.what() + R"("})");
		logger->error("Failed to parse or handle message: {}", e.what());
	}
}