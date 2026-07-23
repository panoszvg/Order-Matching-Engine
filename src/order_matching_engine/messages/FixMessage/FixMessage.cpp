#include "FixMessage.h"
#include <string>
#include <sstream>
#include <iomanip>

void FixMessage::populate(const string& message) {
	rawMessage = message;

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

bool FixMessage::hasField(int tag) const {
	return !getValue(tag).empty();
}

bool FixMessage::checksumValid() const {
	auto checksumPos = rawMessage.rfind(string(1, SOH) + "10=");
	std::size_t bodyEnd;

	if (checksumPos != string::npos) {
		bodyEnd = checksumPos + 1; // keep the SOH that precedes "10="
	} else if (rawMessage.rfind("10=", 0) == 0) {
		bodyEnd = 0; // message starts with the checksum field
	} else {
		return false; // no checksum field present
	}

	unsigned int sum = 0;
	for (std::size_t i = 0; i < bodyEnd; ++i) {
		sum += static_cast<unsigned char>(rawMessage[i]);
	}

	std::ostringstream expected;
	expected << std::setw(3) << std::setfill('0') << (sum % 256);

	return getValue(10) == expected.str();
}

bool FixMessage::requiredFieldsPresent() const {
	static const int requiredTags[] = {8, 35, 55, 54, 38, 44};
	for (int tag : requiredTags) {
		if (!hasField(tag)) return false;
	}
	return true;
}

bool FixMessage::isValid() const {
	return requiredFieldsPresent() && checksumValid();
}

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


