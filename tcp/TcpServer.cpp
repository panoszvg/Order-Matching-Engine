#include "TcpServer.h"
#include "TcpSession.h"
#include <boost/asio/post.hpp>
#include <iostream>

TcpServer::TcpServer(boost::asio::io_context& io_context, const std::vector<int>& ports, std::shared_ptr<IMessageHandler> handler)
	: io_context_(io_context), handler_(std::move(handler)) {
	for (int port : ports) {
		auto acceptor = std::make_unique<tcp::acceptor>(io_context_, tcp::endpoint(tcp::v4(), port));
		std::cout << "Server listening on port " << port << "..." << std::endl;
		acceptors_.emplace_back(std::move(acceptor));
	}
}

void TcpServer::run() {
	for (auto& acceptor : acceptors_) {
		startAccept(*acceptor);
	}
}

void TcpServer::startAccept(tcp::acceptor& acceptor) {
	auto socket = std::make_shared<tcp::socket>(io_context_);

	acceptor.async_accept(*socket, [this, &acceptor, socket](boost::system::error_code ec) {
		if (!ec) {
			std::cout << "Client connected on port " << acceptor.local_endpoint().port() << ".\n";
			std::make_shared<TcpSession>(std::move(*socket), handler_)->start();
		} else {
			std::cerr << "Accept failed on port " << acceptor.local_endpoint().port() << ": " << ec.message() << std::endl;
		}

		boost::asio::post(acceptor.get_executor(), [this, &acceptor]() {
			startAccept(acceptor);
		});
	});
}