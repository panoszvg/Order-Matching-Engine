#include "FixOrderHandler.h"
#include "messages/FixMessage/FixMessage.h"

FixOrderHandler::FixOrderHandler(std::unordered_map<std::string, std::unique_ptr<Book>>& books)
	: books_(books) {}

void FixOrderHandler::handle(const std::string& rawMessage, ISession& session) {
	try {
		FixMessage fixMsg;
		fixMsg.populate(rawMessage);

		if (!fixMsg.isValid()) {
			logger->error("Invalid FIX message: '{}'", rawMessage);
			session.send(R"({"status":"error","message":"Invalid FIX message"})");
			return;
		}

		auto order = fixMsg.makeOrder();

		if (books_.count(order.security)) {
			try {
				books_.at(order.security)->insertOrder(order);
				session.send(R"({"status":"ok","message":"Fix order accepted"})");
			} catch (const std::invalid_argument& arg) {
				logger->error("FIX order rejected: {}", arg.what());
				session.send(std::string(R"({"status":"error","message":")") + arg.what() + R"("})");
			}
		} else {
			logger->error("Unknown security in FIX order: {}", order.security);
			session.send(R"({"status":"error","message":"Unknown security in FIX order"})");
		}
		
	} catch (const std::exception& e) {
		logger->error("Failed to parse FIX message: {} | Message : '{}'", e.what(), rawMessage);
		session.send(std::string(R"({"status":"error","message":")") + e.what() + R"("})");
	}
}
