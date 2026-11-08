#include "FixMessage.h"
#include <string>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <limits>

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

// Tags the engine reads: header, trailer, and the NewOrderSingle fields used by
// isValid()/makeOrder(). A repeat of one of these is ambiguous and is rejected.
// Any other tag may legitimately repeat (FIX repeating groups, e.g. Parties 448/447/452);
// the engine ignores those, so only the first occurrence is kept.
static bool isSignificantTag(int tag) {
	if (tag == 10 || isHeaderTag(tag)) return true;
	switch (tag) {
		case 11:  // ClOrdID
		case 38:  // OrderQty
		case 40:  // OrdType
		case 44:  // Price
		case 54:  // Side
		case 55:  // Symbol
		case 60:  // TransactTime
		case 99:  // StopPx
			return true;
		default:
			return false;
	}
}

enum class TagParse { Ok, Invalid, OutOfRange };

// A FIX tag is a positive integer written in plain digits: no sign, whitespace or leading zero.
// Stricter than std::stoi, which would accept " 58", "+38" and "038" as aliases.
static TagParse parseTag(const string& s, int& tag) {
	constexpr long long maxTag = std::numeric_limits<int>::max();
	if (s.empty() || s[0] == '0') return TagParse::Invalid;
	long long value = 0;
	for (char c : s) {
		if (c < '0' || c > '9') return TagParse::Invalid;
		if (value <= maxTag) value = value * 10 + (c - '0'); // stop growing once out of range
	}
	if (value > maxTag) return TagParse::OutOfRange;
	tag = static_cast<int>(value);
	return TagParse::Ok;
}

// Client text quoted in an error reason is capped, so an oversized field isn't echoed back in full.
static string excerpt(const string& s) {
	constexpr std::size_t maxLen = 64;
	if (s.size() <= maxLen) return s;
	return s.substr(0, maxLen) + "...";
}

// populate() files every tag here, so lookups only ever need to search this one section
const FixMap& FixMessage::sectionFor(int tag) const {
	if (tag == 10) return trailer;
	if (isHeaderTag(tag)) return header;
	return body;
}

void FixMessage::populate(const string& message) {
	// Start from a clean slate so a reused object doesn't mix in a previous message
	header = FixMap();
	body = FixMap();
	trailer = FixMap();
	parseError.clear();

	// Line-oriented clients (telnet, Windows, Python) may leave a trailing '\r' or '\n'
	// after the final SOH; it is transport framing, not part of the FIX message.
	rawMessage = message;
	while (!rawMessage.empty() && (rawMessage.back() == '\r' || rawMessage.back() == '\n')) {
		rawMessage.pop_back();
	}

	std::stringstream ss(rawMessage);
	string token;

	// Keep only the first problem as the reason for rejection and count the rest,
	// so a frame full of bad fields produces one log line instead of thousands
	std::size_t errorCount = 0;
	auto recordError = [this, &errorCount](const string& reason) {
		if (errorCount++ == 0) parseError = reason;
	};

	while (std::getline(ss, token, SOH)) {
		auto equalPos = token.find('=');
		if (equalPos == string::npos) {
			recordError("Field without '=': '" + excerpt(token) + "'");
			continue;
		}

		string tagStr = token.substr(0, equalPos);
		string value = token.substr(equalPos + 1);

		int tag = 0;
		switch (parseTag(tagStr, tag)) {
			case TagParse::Ok:
				break;
			case TagParse::Invalid:
				recordError("Invalid tag '" + excerpt(tagStr) + "'");
				continue;
			case TagParse::OutOfRange:
				recordError("Out of range tag '" + excerpt(tagStr) + "'");
				continue;
		}

		if (value.empty()) {
			recordError("Empty value for tag " + std::to_string(tag));
			continue;
		}

		// Same rule as sectionFor(); a tag always maps to one section, so only that one can hold a duplicate
		FixMap& section = (tag == 10) ? trailer : (isHeaderTag(tag) ? header : body);
		if (section.getFields().count(tag)) {
			if (isSignificantTag(tag)) {
				recordError("Duplicate tag " + std::to_string(tag));
			}
			continue; // keep the first occurrence
		}

		section.addField(tag, value);
	}

	if (errorCount == 1) {
		logger->error("Malformed FIX message: {}", parseError);
	} else if (errorCount > 1) {
		logger->error("Malformed FIX message: {} (and {} more problems)", parseError, errorCount - 1);
	}
}

const string& FixMessage::getParseError() const {
	return parseError;
}

bool FixMessage::hasField(int tag) const {
	return sectionFor(tag).getFields().count(tag) > 0;
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
	if (checksumPos == string::npos) return false; // no checksum field present
	auto bodyEnd = checksumPos + 1; // keep the SOH that precedes "10="

	unsigned int sum = 0;
	for (std::size_t i = 0; i < bodyEnd; ++i) {
		sum += static_cast<unsigned char>(rawMessage[i]);
	}

	std::ostringstream expected;
	expected << std::setw(3) << std::setfill('0') << (sum % 256);

	return getValue(10) == expected.str();
}

int FixMessage::firstMissingField() const {
	static const int requiredTags[] = {8, 9, 35, 49, 56, 34, 52, 11, 55, 54, 38, 40, 60};
	for (int tag : requiredTags) {
		if (!hasField(tag)) return tag;
	}

	const string orderType = getValue(40);
	if ((orderType == "2" || orderType == "4") && !hasField(44)) return 44;
	if ((orderType == "3" || orderType == "4") && !hasField(99)) return 99;
	return 0;
}

string FixMessage::validate() const {
	if (!headerValid())   return "Garbled message: bad BeginString, BodyLength or field order";
	if (!checksumValid()) return "Garbled message: bad CheckSum";
	if (!parseError.empty()) return parseError;

	const string msgType = getValue(35);
	if (msgType != "D") return "Unsupported MsgType '" + excerpt(msgType) + "': only NewOrderSingle (35=D) is supported";

	if (int missing = firstMissingField()) return "Missing required field " + std::to_string(missing);
	return "";
}

bool FixMessage::isValid() const {
	return validate().empty();
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
	const auto& fields = sectionFor(tag).getFields();
	auto it = fields.find(tag);
	return it != fields.end() ? it->second.getValue() : "";
}

void FixMessage::print() {
	auto printSection = [this](const FixMap& section) {
		for (const auto& [tag, field] : section.getFields()) {
			logger->info("Tag {} = {}", tag, field.getValue());
		}
	};
	printSection(header);
	printSection(body);
	printSection(trailer);
}

