#ifndef ISESSION_H
#define ISESSION_H

#pragma once
#include <string>

class ISession {
public:
	virtual ~ISession() = default;
	virtual void send(const std::string& message) = 0;
};

#endif // ISESSION_H
