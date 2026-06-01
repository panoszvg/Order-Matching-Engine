#ifndef ISIMPLEMESSAGE_H
#define ISIMPLEMESSAGE_H

#include "messages/IMessage.h"
#include <unordered_map>

class SimpleMessage : public IMessage {
private:
	std::unordered_map<int, string> fields;

public:
	void populate(const string& message) override;
	void isValid() override;
	Order makeOrder() override;
};

#endif // ISIMPLEMESSAGE_H