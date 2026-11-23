#include "FixFramer.h"
#include "messages/IMessage.h"
#include <algorithm>

namespace {

constexpr std::string_view prefix = "8=FIX.4.4\x01" "9=";
constexpr std::size_t trailerSize = 7; // "10=" + 3-digit CheckSum + SOH

bool isDigit(char c) { return c >= '0' && c <= '9'; }

bool isTrailer(std::string_view t) {
	return t.size() == trailerSize && t.compare(0, 3, "10=") == 0
		&& isDigit(t[3]) && isDigit(t[4]) && isDigit(t[5]) && t[6] == SOH;
}

// Whether `s` matches the prefix as far as `s` goes (all of it, if `s` is long enough)
bool matchesPrefix(std::string_view s) {
	const std::size_t n = std::min(s.size(), prefix.size());
	return s.compare(0, n, prefix.substr(0, n)) == 0;
}

} // namespace

Frame FixFramer::next(std::string_view buffered) const {
	// 1. BeginString and the start of BodyLength; judge a partial prefix as far as it goes
	if (!matchesPrefix(buffered)) return Frame::invalid("Message does not start with 8=FIX.4.4<SOH>9=");
	if (buffered.size() < prefix.size()) return Frame::incomplete();

	// 2. BodyLength digits, up to the SOH that ends tag 9
	std::size_t pos = prefix.size();
	std::size_t bodyLength = 0;
	for (;; ++pos) {
		if (pos == buffered.size()) return Frame::incomplete();
		const char c = buffered[pos];
		if (c == SOH) break;
		if (!isDigit(c)) return Frame::invalid("BodyLength is not a number");
		// Zeros would never reach the cap below, so a client could send them forever
		if (c == '0' && pos == prefix.size()) return Frame::invalid("BodyLength has a leading zero");
		bodyLength = bodyLength * 10 + (c - '0');
		// A huge BodyLength would have the session wait for, and buffer, bytes that never come
		if (bodyLength > kMaxBodyLength) return Frame::invalid("BodyLength over " + std::to_string(kMaxBodyLength));
	}
	if (pos == prefix.size()) return Frame::invalid("BodyLength is empty");

	// 3. exactly BodyLength bytes, then the trailer
	const std::size_t bodyStart = pos + 1;
	const std::size_t total = bodyStart + bodyLength + trailerSize;
	if (buffered.size() < total) return Frame::incomplete();

	if (!isTrailer(buffered.substr(bodyStart + bodyLength, trailerSize))) {
		return Frame::invalid("No CheckSum (10=xxx) where BodyLength says the body ends");
	}
	return Frame::complete(total, std::string(buffered.substr(0, total)));
}
