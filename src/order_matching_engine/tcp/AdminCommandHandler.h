#ifndef ADMINCOMMANDHANDLER_H
#define ADMINCOMMANDHANDLER_H

#include "IMessageHandler.h"
#include "TcpSession.h"
#include "../Book.h"
#include "../Order.h"
#include "../Logger.h"
#include <nlohmann/json.hpp>
#include <memory>
#include <unordered_map>

using json = nlohmann::json;

class AdminCommandHandler : public IMessageHandler {
private:
	std::unordered_map<std::string, std::shared_ptr<Book>>& books_;
	
public:
	AdminCommandHandler(std::unordered_map<std::string, std::shared_ptr<Book>>& books);

	void handle(const std::string& rawMessage, TcpSession& session) override;
};


#endif // ADMINCOMMANDHANDLER_H