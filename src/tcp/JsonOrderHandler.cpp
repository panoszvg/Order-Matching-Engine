#include "JsonOrderHandler.h"

JsonOrderHandler::JsonOrderHandler(std::unordered_map<std::string, std::shared_ptr<Book>>& books)
	: books_(books) {}

void JsonOrderHandler::handle(const std::string& rawMessage, TcpSession& session) {
	try {
		auto j = json::parse(rawMessage);

		std::string security = j.at("security").get<std::string>();
		std::string typeStr = j.at("type").get<std::string>();
		double price = j.at("price").get<double>();
		double quantity = j.at("quantity").get<double>();

		OrderType type = (typeStr == "BUY") ? BUY : SELL;

		auto order = std::make_shared<Order>(security, type, quantity, price);

		if (books_.count(security)) {
			try {
				books_.at(security)->insertOrder(order);
				session.send(R"({"status":"ok","message":"Order accepted"})");
			} catch (const std::invalid_argument& arg) {
				logger->error("Order rejected: {}", arg.what());
				session.send(std::string(R"({"status":"error","message":")") + arg.what() + R"("})");
			}
		} else {
			logger->error("Unknown security: {}", security);
			session.send(R"({"status":"error","message":"Unknown security"})");
		}

	} catch (const std::exception& e) {
		logger->error("Failed to parse or handle message: {}", e.what());
		session.send(std::string(R"({"status":"error","message":")") + e.what() + R"("})");
	}
}