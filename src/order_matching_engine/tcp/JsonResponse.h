#ifndef JSONRESPONSE_H
#define JSONRESPONSE_H

#pragma once

#include "ISession.h"
#include <nlohmann/json.hpp>
#include <string>

// Every reply goes through nlohmann::json so that client-supplied text
// (symbols, field values, exception messages that echo them) is always escaped.
// Client text may also be invalid UTF-8 (a raw 0xFF, or a multi-byte character cut by
// a truncated excerpt); dump() throws on that by default, so bad bytes are replaced with U+FFFD.

inline void sendResponse(ISession& session, const std::string& status, const std::string& message) {
	nlohmann::json response = {{"status", status}, {"message", message}};
	session.send(response.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace));
}

inline void sendOk(ISession& session, const std::string& message) {
	sendResponse(session, "ok", message);
}

inline void sendError(ISession& session, const std::string& message) {
	sendResponse(session, "error", message);
}

#endif // JSONRESPONSE_H
