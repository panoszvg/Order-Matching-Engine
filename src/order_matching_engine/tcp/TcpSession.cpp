#include "TcpSession.h"
#include "Logger.h"

void TcpSession::start() {
	read();
}

void TcpSession::read() {
	auto self = shared_from_this();
	boost::asio::async_read_until(socket_, buffer_, '\n',
		[self](boost::system::error_code ec, std::size_t) {
			if (!ec) {
				std::istream input(&self->buffer_);
				std::string message;
				std::getline(input, message);

				if (self->handler_) {
					self->handler_->handle(message, *self);
				}

				self->read(); // Continue reading
			} else {
				netLogger->info("Client disconnected: {}", ec.message());
			}
		}
	);
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
			}
		});
}
