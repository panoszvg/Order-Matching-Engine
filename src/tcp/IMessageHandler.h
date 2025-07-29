#ifndef IMESSAGEHANDLER_H
#define IMESSAGEHANDLER_H

#pragma once
#include "TcpSession.h"
#include <string>

class IMessageHandler {
public:
	virtual ~IMessageHandler() = default;
	virtual void handle(const std::string& rawMessage, TcpSession& session) = 0;
};

#endif // IMESSAGEHANDLER_H