#include "SimpleMessage.h"
#include <string>
#include <sstream>

using std::make_shared;

void SimpleMessage::populate(const string& message) {
	std::stringstream ss(message);
	string token;

	while (std::getline(ss, token, SOH)) {
		auto equalPos = token.find('=');
		if (equalPos != string::npos) {
			int tag = stoi(token.substr(0, equalPos));
			string value = token.substr(equalPos + 1);
			fields[tag] = value;
		}
	}
}

void SimpleMessage::isValid() {}

shared_ptr<Order> SimpleMessage::makeOrder() {
	string  tag_55 = fields[55];
	int     tag_54 = stoi(fields[54]);
	double  tag_38 = stod(fields[38]);
	double  tag_44 = stod(fields[44]);

	shared_ptr<Order> newOrder = make_shared<Order>(tag_55, (tag_54 == 1) ? BUY : SELL, tag_38, tag_44);
	return newOrder;
}