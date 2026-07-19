#ifndef IMESSAGEHANDLER_H
#define IMESSAGEHANDLER_H

#pragma once
#include <string>
#include "ISession.h"

class IMessageHandler {
public:
	virtual ~IMessageHandler() = default;
	virtual void handle(const std::string& rawMessage, ISession& session) = 0;
};

#endif // IMESSAGEHANDLER_H