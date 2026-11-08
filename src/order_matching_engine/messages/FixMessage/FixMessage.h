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
	string rawMessage;
	string parseError; // first problem found by populate(); empty if none

	const FixMap& sectionFor(int tag) const;
	bool hasField(int tag) const;
	bool headerValid() const;
	bool checksumValid() const;
	int firstMissingField() const; // 0 if every required field is present

public:
	void populate(const string& message) override;
	bool isValid() const override;
	string validate() const; // reason for the first failed check; empty if the message is valid
	Order makeOrder() override;
	string getValue(int tag) const;
	const string& getParseError() const;

	void print();
};

#endif // IFIXMESSAGE_H
