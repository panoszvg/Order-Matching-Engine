#include "JsonOrderHandler.h"
#include "JsonResponse.h"

JsonOrderHandler::JsonOrderHandler(std::unordered_map<std::string, std::unique_ptr<Book>>& books)
	: books_(books) {}

void JsonOrderHandler::handle(const std::string& rawMessage, ISession& session) {
	try {
		auto j = json::parse(rawMessage);

		std::string security = j.at("security").get<std::string>();
		std::string typeStr = j.at("type").get<std::string>();
		double price = j.at("price").get<double>();
		double quantity = j.at("quantity").get<double>();

		if (typeStr != "BUY" && typeStr != "SELL") {
			logger->error("Unknown type: {}", typeStr);
			sendError(session, "Unknown type");
			return;
		}
		OrderType type = (typeStr == "BUY") ? BUY : SELL;

		auto order = std::make_unique<Order>(security, type, quantity, price);

		if (books_.count(security)) {
			try {
				books_.at(security)->insertOrder(*order);
				sendOk(session, "Order accepted");
			} catch (const std::invalid_argument& arg) {
				logger->error("Order rejected: {}", arg.what());
				sendError(session, arg.what());
			}
		} else {
			logger->error("Unknown security: {}", security);
			sendError(session, "Unknown security");
		}

	} catch (const std::exception& e) {
		logger->error("Failed to parse or handle message: {}", e.what());
		sendError(session, e.what());
	}
}
