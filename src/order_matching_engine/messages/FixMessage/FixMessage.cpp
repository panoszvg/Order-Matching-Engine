#include "FixMessage.h"
#include <string>
#include <sstream>

void FixMessage::populate(const string& message) {
	std::stringstream ss(message);
	string token;
	FixMap* currentMap = &header;

	while (std::getline(ss, token, SOH)) {
		auto equalPos = token.find('=');
		if (equalPos != string::npos) {
			string tagStr = token.substr(0, equalPos);
			string value = token.substr(equalPos + 1);

			try {
				int tag = std::stoi(tagStr);

				if (tag == 10) currentMap = &trailer;
				currentMap->addField(tag, value);
				if (tag == 35) currentMap = &body;

			} catch (const std::invalid_argument& e) {
				logger->error("Invalid tag in FIX field: '{}'. Skipping.", tagStr);
			} catch (const std::out_of_range& e) {
				logger->error("Out of range tag in FIX field: '{}'. Skipping.", tagStr);
			}
		}
	}

}

void FixMessage::isValid() {}

Order FixMessage::makeOrder() {
	try {
		string  tag_55 = this->getValue(55);
		int     tag_54 = stoi(this->getValue(54));
		double  tag_38 = stod(this->getValue(38));
		double  tag_44 = stod(this->getValue(44));

		return Order(tag_55, (tag_54 == 1) ? BUY : SELL, tag_38, tag_44);
	} catch (const std::exception& e) {
		logger->error("FIX makeOrder failed: {}", e.what());
		throw;
	}
}

string FixMessage::getValue(int tag) const {
	for (const FixMap* section : {&header, &body, &trailer}) {
		const auto& fields = section->getFields();
		auto it = fields.find(tag);
		if (it != fields.end()) return it->second.getValue();
	}
	return "";
}

void FixMessage::print() {
	for (const FixMap* section : {&header, &body, &trailer}) {
		for (const auto& [tag, field] : section->getFields()) {
			logger->info("Tag {} = {}", tag, field.getValue());
		}
	}
}


