#ifndef FIXFRAMER_H
#define FIXFRAMER_H

#pragma once
#include "IFramer.h"

// FIX messages carry their own length, so no delimiter is needed between them:
//   1. "8=FIX.4.4<SOH>9=" starts every message
//   2. BodyLength (tag 9) runs up to the next SOH
//   3. exactly BodyLength bytes follow, then the 7-byte trailer "10=xxx<SOH>"
// Values may hold any byte, including '\n', and messages may arrive back to back.
// The framer only finds the message's end; FixMessage::validate() still checks its contents.
class FixFramer : public IFramer {
public:
	static constexpr std::size_t kMaxBodyLength = 64 * 1024;

	Frame next(std::string_view buffered) const override;
};

#endif // FIXFRAMER_H
