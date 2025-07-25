#ifndef IMESSAGEHANDLER_H
#define IMESSAGEHANDLER_H

#pragma once
#include <string>

class IMessageHandler {
public:
	virtual ~IMessageHandler() = default;
	virtual void handle(const std::string& rawMessage) = 0;
};

#endif // IMESSAGEHANDLER_H