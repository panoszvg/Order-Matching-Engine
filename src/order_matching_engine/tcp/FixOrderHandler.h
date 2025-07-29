#ifndef FIXORDERHANDLER_H
#define FIXORDERHANDLER_H

#include "../Book.h"
#include "../Logger.h"
#include "IMessageHandler.h"
#include "TcpSession.h"
#include "../messages/FixMessage/FixMessage.h"
#include <memory>
#include <unordered_map>

class FixOrderHandler : public IMessageHandler {
private:
    std::unordered_map<std::string, std::shared_ptr<Book>>& books_;

public:
    explicit FixOrderHandler(std::unordered_map<std::string, std::shared_ptr<Book>>& books);

    void handle(const std::string& rawMessage, TcpSession& session) override;
};

#endif // FIXORDERHANDLER_H