#include "TcpSession.h"

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
					self->handler_->handle(message);
				}

				self->read(); // Continue reading
			} else {
				std::cout << "Client disconnected: " << ec.message() << std::endl;
			}
		}
	);
}
