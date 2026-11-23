#ifndef LINEFRAMER_H
#define LINEFRAMER_H

#pragma once
#include "IFramer.h"

// One message per line, ended by '\n' or "\r\n" (JSON and admin ports). The line ending is dropped.
class LineFramer : public IFramer {
public:
	// Longest message accepted, not counting the line ending
	static constexpr std::size_t kMaxMessageSize = 64 * 1024;

	Frame next(std::string_view buffered) const override;
};

#endif // LINEFRAMER_H
