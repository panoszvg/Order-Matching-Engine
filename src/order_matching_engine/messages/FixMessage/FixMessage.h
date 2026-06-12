#ifndef IFIXMESSAGE_H
#define IFIXMESSAGE_H

#pragma once

#include "Logger.h"
#include "messages/IMessage.h"
#include "FixMap.h"
#include <string>

using std::string;

class FixMessage : public IMessage {
private:
	FixMap header;
	FixMap body;
	FixMap trailer;

public:
	void populate(const string& message) override;
	void isValid() override;
	Order makeOrder() override;
	string getValue(int tag) const;

	void print();
};

#endif // IFIXMESSAGE_H
