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

void TcpSession::send(const std::string& message) {
	auto self = shared_from_this();
	boost::asio::async_write(socket_, boost::asio::buffer(message + '\n'),
		[self](boost::system::error_code ec, std::size_t) {
			if (ec) {
				netLogger->error("Send failed: {}", ec.message());
			}
		});
}
