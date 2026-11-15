#include "TcpServer.h"
#include "TcpSession.h"
#include "Logger.h"
#include <chrono>

using boost::asio::ip::tcp;

namespace {

// How long to wait before accepting again after an error. Errors like running out of file
// descriptors (EMFILE) last until other connections close, so retrying at once would spin.
constexpr std::chrono::milliseconds kAcceptRetryDelay{100};

// For logging only: the error_code overload of local_endpoint() doesn't throw.
unsigned short portOf(const tcp::acceptor& acceptor) {
	boost::system::error_code ec;
	return acceptor.local_endpoint(ec).port();
}

} // namespace

TcpServer::TcpServer(boost::asio::io_context& io_context, const std::vector<int>& ports, std::shared_ptr<IMessageHandler> handler)
	: io_context_(io_context), handler_(std::move(handler)) {
	for (int port : ports) {
		auto acceptor = std::make_unique<tcp::acceptor>(io_context_, tcp::endpoint(tcp::v4(), port));
		netLogger->info("Server listening on port {}...", port);
		acceptors_.emplace_back(std::move(acceptor));
	}
}

void TcpServer::run() {
	for (auto& acceptor : acceptors_) {
		startAccept(*acceptor);
	}
}

void TcpServer::startAccept(tcp::acceptor& acceptor) {
	acceptor.async_accept([this, &acceptor](boost::system::error_code ec, tcp::socket socket) {
		if (ec == boost::asio::error::operation_aborted) {
			return; // The acceptor was closed
		}

		if (ec) {
			netLogger->error("Accept failed on port {}: {}", portOf(acceptor), ec.message());
			auto timer = std::make_shared<boost::asio::steady_timer>(io_context_, kAcceptRetryDelay);
			timer->async_wait([this, &acceptor, timer](boost::system::error_code timerEc) {
				if (!timerEc) {
					startAccept(acceptor);
				}
			});
			return;
		}

		netLogger->info("Client connected on port {}.", portOf(acceptor));
		std::make_shared<TcpSession>(std::move(socket), handler_)->start();
		startAccept(acceptor);
	});
}
