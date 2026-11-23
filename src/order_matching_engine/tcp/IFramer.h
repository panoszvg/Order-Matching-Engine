#ifndef IFRAMER_H
#define IFRAMER_H

#pragma once
#include <cstddef>
#include <string>
#include <string_view>

// What a framer found at the start of the bytes a session has buffered so far.
struct Frame {
	enum class Status {
		Incomplete, // no whole message yet: read more bytes and ask again
		Complete,   // `message` is the next message; drop `consumed` bytes from the buffer
		Invalid     // no message can start here; `message` is the reason
	};

	Status status = Status::Incomplete;
	std::size_t consumed = 0;
	std::string message;

	static Frame incomplete() { return {}; }
	static Frame complete(std::size_t consumed, std::string message) { return {Status::Complete, consumed, std::move(message)}; }
	static Frame invalid(std::string reason) { return {Status::Invalid, 0, std::move(reason)}; }
};

// Decides where one message ends and the next begins on a TCP byte stream.
// Stateless: a session passes in everything it has buffered and drops what each answer consumed.
class IFramer {
public:
	virtual ~IFramer() = default;
	virtual Frame next(std::string_view buffered) const = 0;
};

#endif // IFRAMER_H
