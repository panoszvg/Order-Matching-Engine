#include "SimpleMessage.h"
#include "Logger.h"
#include <string>
#include <sstream>

using std::make_shared;

void SimpleMessage::populate(const string& message) {
	std::stringstream ss(message);
	string token;

	while (std::getline(ss, token, SOH)) {
		auto equalPos = token.find('=');
		if (equalPos != string::npos) {
			string tagStr = token.substr(0, equalPos);
			string value = token.substr(equalPos + 1);

			try {
				int tag = stoi(tagStr);
				fields[tag] = value;
			} catch (const std::invalid_argument& e) {
				logger->error("Invalid tag in Simple message field: '{}'. Skipping.", tagStr);
			} catch (const std::out_of_range& e) {
				logger->error("Out of range tag in Simple message field: '{}'. Skipping.", tagStr);
			}
		}
	}
}

void SimpleMessage::isValid() {}

Order SimpleMessage::makeOrder() {
	string  tag_55 = fields[55];
	int     tag_54 = stoi(fields[54]);
	double  tag_38 = stod(fields[38]);
	double  tag_44 = stod(fields[44]);

	return Order(tag_55, (tag_54 == 1) ? BUY : SELL, tag_38, tag_44);
}