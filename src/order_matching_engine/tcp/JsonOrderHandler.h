#ifndef JSONMESSAGEHANDLER_H
#define JSONMESSAGEHANDLER_H

#include "IMessageHandler.h"
#include "TcpSession.h"
#include "../Book.h"
#include "../Order.h"
#include "../Logger.h"
#include <nlohmann/json.hpp>
#include <memory>
#include <unordered_map>

using json = nlohmann::json;

class JsonOrderHandler : public IMessageHandler {
private:
	std::unordered_map<std::string, std::shared_ptr<Book>>& books_;
	
public:
	JsonOrderHandler(std::unordered_map<std::string, std::shared_ptr<Book>>& books);

	void handle(const std::string& rawMessage, TcpSession& session) override;
};


#endif // JSONMESSAGEHANDLER_H