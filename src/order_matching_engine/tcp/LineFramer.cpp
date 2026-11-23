#include "LineFramer.h"

Frame LineFramer::next(std::string_view buffered) const {
	const auto newline = buffered.find('\n');
	std::size_t length = (newline == std::string_view::npos) ? buffered.size() : newline;
	if (length > 0 && newline != std::string_view::npos && buffered[length - 1] == '\r') {
		--length; // Line ended with "\r\n"
	}

	// Checked before the newline arrives too, or a client that never sends one would grow the buffer forever.
	// Without a newline yet, a trailing '\r' may be the start of "\r\n", so it is allowed one byte of slack.
	const std::size_t limit = (newline == std::string_view::npos) ? kMaxMessageSize + 1 : kMaxMessageSize;
	if (length > limit) {
		return Frame::invalid("Message longer than " + std::to_string(kMaxMessageSize) + " bytes");
	}

	if (newline == std::string_view::npos) return Frame::incomplete();
	return Frame::complete(newline + 1, std::string(buffered.substr(0, length)));
}
