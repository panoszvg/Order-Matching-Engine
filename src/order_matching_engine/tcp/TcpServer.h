#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <boost/asio.hpp>
#include <memory>
#include "IMessageHandler.h"

class TcpServer {
private:
	boost::asio::io_context& io_context_;
	std::vector<std::unique_ptr<boost::asio::ip::tcp::acceptor>> acceptors_;
	std::shared_ptr<IMessageHandler> handler_;

	void startAccept(boost::asio::ip::tcp::acceptor& acceptor);

public:
	TcpServer(boost::asio::io_context& io_context, const std::vector<int>& ports, std::shared_ptr<IMessageHandler> handler);
	void run();
};

#endif // TCPSERVER_H