#include "TcpSession.h"
#include "JsonResponse.h"
#include "Logger.h"
#include <exception>
#include <string_view>

void TcpSession::start() {
	read();
}

void TcpSession::read() {
	auto self = shared_from_this();
	socket_.async_read_some(boost::asio::buffer(readBuffer_),
		[self](boost::system::error_code ec, std::size_t bytesRead) {
			if (ec) {
				// No more reads, so the session closes once any pending replies are written
				netLogger->info("Client disconnected: {}", ec.message());
				return;
			}
			if (!self->framingFailed_) {
				self->pending_.append(self->readBuffer_.data(), bytesRead);
				self->handlePending();
			}
			self->read(); // Continue reading
		}
	);
}

// One read may hold part of a message, exactly one, or several back to back
void TcpSession::handlePending() {
	std::size_t offset = 0;

	while (offset < pending_.size()) {
		Frame frame = framer_->next(std::string_view(pending_).substr(offset));

		if (frame.status == Frame::Status::Incomplete) break;

		if (frame.status == Frame::Status::Invalid) {
			netLogger->error("Framing error: {}", frame.message);
			sendError(*this, "Framing error: " + frame.message);
			framingFailed_ = true;
			pending_.clear();
			return;
		}

		offset += frame.consumed;
		handleMessage(frame.message);
	}

	pending_.erase(0, offset);
}

// The handlers catch their own errors; this is the last line of defence, because an exception
// escaping here would leave io_context.run() and stop the engine for every client
void TcpSession::handleMessage(const std::string& message) {
	if (!handler_) return;
	try {
		handler_->handle(message, *this);
	} catch (const std::exception& e) {
		netLogger->error("Unhandled error while handling a message: {}", e.what());
		sendError(*this, "Internal error");
	} catch (...) {
		netLogger->error("Unhandled non-standard exception while handling a message");
		sendError(*this, "Internal error");
	}
}

// Only one async_write may be in progress on a socket at a time, so replies are queued
// and written one after another.
void TcpSession::send(const std::string& message) {
	bool writing = !writeQueue_.empty();
	writeQueue_.push_back(message + '\n');
	if (!writing) {
		write();
	}
}

void TcpSession::write() {
	auto self = shared_from_this();
	boost::asio::async_write(socket_, boost::asio::buffer(writeQueue_.front()),
		[self](boost::system::error_code ec, std::size_t) {
			if (ec) {
				netLogger->error("Send failed: {}", ec.message());
				// Drop unsent replies and close, which also ends the pending read and the session
				self->writeQueue_.clear();
				boost::system::error_code ignored;
				self->socket_.close(ignored);
				return;
			}
			self->writeQueue_.pop_front();
			if (!self->writeQueue_.empty()) {
				self->write();
			} else if (self->framingFailed_) {
				// The error reply is out: tell the client we're done. Shutting down only the sending
				// side, not closing, means its unread input can't make the error reply get lost in a reset.
				boost::system::error_code ignored;
				self->socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ignored);
			}
		});
}
