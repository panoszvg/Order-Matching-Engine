#include "FixMessage.h"
#include <string>
#include <sstream>
#include <iomanip>
#include <stdexcept>

// FIX 4.4 StandardHeader fields. 8, 9, 35 must come first (checked in headerValid);
// the rest may appear in any order before the body.
static bool isHeaderTag(int tag) {
	switch (tag) {
		case 8:   // BeginString
		case 9:   // BodyLength
		case 35:  // MsgType
		case 49:  // SenderCompID
		case 56:  // TargetCompID
		case 34:  // MsgSeqNum
		case 52:  // SendingTime
		case 43:  // PossDupFlag
		case 97:  // PossResend
		case 115: // OnBehalfOfCompID
		case 128: // DeliverToCompID
		case 122: // OrigSendingTime
		case 50:  // SenderSubID
		case 57:  // TargetSubID
			return true;
		default:
			return false;
	}
}

void FixMessage::populate(const string& message) {
	rawMessage = message;

	std::stringstream ss(message);
	string token;

	while (std::getline(ss, token, SOH)) {
		auto equalPos = token.find('=');
		if (equalPos != string::npos) {
			string tagStr = token.substr(0, equalPos);
			string value = token.substr(equalPos + 1);

			try {
				int tag = std::stoi(tagStr);

				if (tag == 10) {
					trailer.addField(tag, value);
				} else if (isHeaderTag(tag)) {
					header.addField(tag, value);
				} else {
					body.addField(tag, value);
				}

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

bool FixMessage::headerValid() const {
	const string prefix = "8=FIX.4.4" + string(1, SOH) + "9=";
	if (rawMessage.compare(0, prefix.size(), prefix) != 0) return false;

	auto bodyStart = rawMessage.find(SOH, prefix.size());
	auto checksumPos = rawMessage.rfind(string(1, SOH) + "10=");
	if (bodyStart == string::npos || checksumPos == string::npos || checksumPos < bodyStart) return false;

	++bodyStart; // first byte after the BodyLength field
	if (rawMessage.compare(bodyStart, 3, "35=") != 0) return false;

	auto bodyEnd = checksumPos + 1; // keep the SOH that precedes "10="
	return getValue(9) == std::to_string(bodyEnd - bodyStart);
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
	static const int requiredTags[] = {8, 9, 35, 49, 56, 34, 52, 11, 55, 54, 38, 40, 60};
	for (int tag : requiredTags) {
		if (!hasField(tag)) return false;
	}

	const string orderType = getValue(40);
	if ((orderType == "2" || orderType == "4") && !hasField(44)) return false;
	if ((orderType == "3" || orderType == "4") && !hasField(99)) return false;
	return true;
}

bool FixMessage::isValid() const {
	return headerValid() && getValue(35) == "D" && requiredFieldsPresent() && checksumValid();
}

Order FixMessage::makeOrder() {
	try {
		// Valid FIX, but the engine only matches limit orders
		if (getValue(40) != "2") {
			throw std::invalid_argument("Unsupported OrderType '" + getValue(40) + "': only limit orders (40=2) are supported");
		}

		string  tag_55 = this->getValue(55);
		int     tag_54 = stoi(this->getValue(54));
		if (tag_54 != 1 && tag_54 != 2) {
			throw std::invalid_argument("Unsupported Side '" + getValue(54) + "': only buy (1) and sell (2) are supported");
		}
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


