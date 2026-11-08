#include "FixOrderHandler.h"
#include "JsonResponse.h"
#include "messages/FixMessage/FixMessage.h"

// Cap how much of a client message is written to the log, so one oversized frame can't flood it.
static std::string logExcerpt(const std::string& raw) {
	constexpr std::size_t maxLen = 512;
	if (raw.size() <= maxLen) return raw;
	return raw.substr(0, maxLen) + "... (" + std::to_string(raw.size()) + " bytes)";
}

FixOrderHandler::FixOrderHandler(std::unordered_map<std::string, std::unique_ptr<Book>>& books)
	: books_(books) {}

void FixOrderHandler::handle(const std::string& rawMessage, ISession& session) {
	try {
		FixMessage fixMsg;
		fixMsg.populate(rawMessage);

		const std::string reason = fixMsg.validate();
		if (!reason.empty()) {
			logger->error("Invalid FIX message ({}): '{}'", reason, logExcerpt(rawMessage));
			sendError(session, "Invalid FIX message: " + reason);
			return;
		}

		auto order = fixMsg.makeOrder();

		if (books_.count(order.security)) {
			try {
				books_.at(order.security)->insertOrder(order);
				sendOk(session, "Fix order accepted");
			} catch (const std::invalid_argument& arg) {
				logger->error("FIX order rejected: {}", arg.what());
				sendError(session, arg.what());
			}
		} else {
			logger->error("Unknown security in FIX order: {}", order.security);
			sendError(session, "Unknown security in FIX order");
		}

	} catch (const std::exception& e) {
		logger->error("Failed to parse FIX message: {} | Message : '{}'", e.what(), logExcerpt(rawMessage));
		sendError(session, e.what());
	}
}
