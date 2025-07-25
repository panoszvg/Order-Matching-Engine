#include "FixOrderHandler.h"
#include "../messages/FixMessage/FixMessage.h"

FixOrderHandler::FixOrderHandler(std::unordered_map<std::string, std::shared_ptr<Book>>& books)
    : books_(books) {}

void FixOrderHandler::handle(const std::string& rawMessage) {
    try {
        FixMessage fixMsg;
        fixMsg.populate(rawMessage);

        auto order = fixMsg.makeOrder();

        if (books_.count(order->security)) {
            try {
                books_.at(order->security)->insertOrder(order);
            } catch (const std::invalid_argument& arg) {
                logger->error("FIX order rejected: {}", arg.what());
            }
        } else {
            logger->error("Unknown security in FIX order: {}", order->security);
        }

    } catch (const std::exception& e) {
        logger->error("Failed to parse FIX message: {} | Message : '{}'", e.what(), rawMessage);
    }
}
