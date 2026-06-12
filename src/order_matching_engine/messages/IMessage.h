#ifndef IMESSAGE_H
#define IMESSAGE_H

#include "Order.h"
#include <memory>

constexpr char SOH = '\x01';

class IMessage {
public:
	virtual void populate(const string& message) = 0;
	virtual void isValid() = 0;
	virtual Order makeOrder() = 0;
	virtual ~IMessage() = default;
};

#endif // IMESSAGE_H
