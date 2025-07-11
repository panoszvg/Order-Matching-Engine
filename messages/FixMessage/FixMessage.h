#ifndef IFIXMESSAGE_H
#define IFIXMESSAGE_H

#pragma once

#include "../../Logger.h"
#include "../IMessage.h"
#include "FixMap.h"
#include <map>
#include <memory>
#include <string>

using std::map;
using std::string;
using std::shared_ptr;


class FixMessage : public IMessage {
private:
	shared_ptr<FixMap> header;
	shared_ptr<FixMap> body;
	shared_ptr<FixMap> trailer;

public:
	FixMessage();
	void populate(const string& message) override;
	void isValid() override;
	shared_ptr<Order> makeOrder() override;
	string getValue(int tag);

	void print();
};

#endif // IFIXMESSAGE_H
