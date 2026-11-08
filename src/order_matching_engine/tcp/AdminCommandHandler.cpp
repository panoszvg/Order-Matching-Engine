#include "AdminCommandHandler.h"
#include "JsonResponse.h"

AdminCommandHandler::AdminCommandHandler(std::unordered_map<std::string, std::unique_ptr<Book>>& books)
	: books_(books) {}

Book* AdminCommandHandler::findBook(const std::string& symbol, ISession& session) {
	auto it = books_.find(symbol);
	if (it == books_.end()) {
		sendError(session, "Unknown symbol");
		return nullptr;
	}
	return it->second.get();
}

void AdminCommandHandler::handle(const std::string& rawMessage, ISession& session) {
	try {
		auto json = nlohmann::json::parse(rawMessage);
		std::string type = json["type"];

		if (type == "GET_SNAPSHOT") {
			try {
				std::string symbol = json["symbol"];
				Book* book = findBook(symbol, session);
				if (book) {
					nlohmann::json response = {
						{"status", "ok"},
						{"message", "Snapshot for " + symbol},
						{"symbol", symbol},
						{"book", book->toJson()}
					};
					session.send(response.dump());
					logger->info("Book was exported and returned");
				}
			} catch (const std::exception& e) {
				sendError(session, std::string("Exception: ") + e.what());
				logger->error("Failed export book: {}", e.what());
			}
		}
		else if (type == "EXPORT_SNAPSHOT") {
			try {
				std::string symbol = json["symbol"];
				Book* book = findBook(symbol, session);
				if (book) {
					book->exportSnapshot();
					sendOk(session, "Exported snapshot for " + symbol);
					logger->info("Book was exported");
				}
			} catch (const std::exception& e) {
				sendError(session, std::string("Exception: ") + e.what());
				logger->error("Failed export book: {}", e.what());
			}
		} else {
			sendError(session, "Unknown command");
		}

	} catch (const std::exception& e) {
		sendError(session, std::string("Invalid JSON or command: ") + e.what());
		logger->error("Failed to parse or handle message: {}", e.what());
	}
}
