#ifndef IMESSAGE_H
#define IMESSAGE_H

#include "../Order.h"
#include <memory>

using std::shared_ptr;

constexpr char SOH = '\x01';

class IMessage {
public:
	virtual void populate(const string& message) = 0;
	virtual void isValid() = 0;
	virtual ~IMessage() = default;
};

#endif // IMESSAGE_H
