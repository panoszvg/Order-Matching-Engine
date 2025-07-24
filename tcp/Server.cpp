#include "Server.h"

TcpServer::TcpServer(boost::asio::io_context& io_context, int port, std::shared_ptr<IMessageHandler> handler)
	: acceptor_(io_context, tcp::endpoint(tcp::v4(), port)), handler_(std::move(handler)) {
	std::cout << "Server listening on port " << port << "..." << std::endl;
}

void TcpServer::startAccept() {
    socket_ = std::make_unique<tcp::socket>(acceptor_.get_executor());

	auto self = shared_from_this();
    acceptor_.async_accept(*socket_, [self](boost::system::error_code ec) {
        if (!ec) {
            std::cout << "Client connected." << std::endl;
            self->handleClient();
        } else {
            std::cerr << "Accept failed: " << ec.message() << std::endl;
        }
    });
}

void TcpServer::handleClient() {
    buffer_ = std::make_unique<boost::asio::streambuf>();
	auto self = shared_from_this();
    boost::asio::async_read_until(*socket_, *buffer_, '\n',
        [self](boost::system::error_code ec, std::size_t bytes_transferred) {
            if (!ec) {
                std::istream input(self->buffer_.get());
                std::string message;
                std::getline(input, message);

                if (self->handler_) {
                    self->handler_->handle(message);
                }

                self->handleClient();  // keep reading
            } else {
                std::cout << "Client disconnected: " << ec.message() << std::endl;
                self->startAccept();  // accept new connection
            }
        }
    );
}
