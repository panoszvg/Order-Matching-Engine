#pragma once

#include "tcp/ISession.h"
#include <string>
#include <vector>

class FakeSession : public ISession {
public:
	std::vector<std::string> sent;

	void send(const std::string& message) override {
		sent.push_back(message);
	}
};
