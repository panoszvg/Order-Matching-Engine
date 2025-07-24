#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <iostream>
#include <boost/asio.hpp>
#include "IMessageHandler.h"

using boost::asio::ip::tcp;

class TcpServer : public std::enable_shared_from_this<TcpServer> {
private:
    boost::asio::ip::tcp::acceptor acceptor_;
	std::unique_ptr<boost::asio::ip::tcp::socket> socket_;
    std::unique_ptr<boost::asio::streambuf> buffer_;
    std::shared_ptr<IMessageHandler> handler_;

	void handleClient();
	
public:
	TcpServer(boost::asio::io_context& io_context, int port, std::shared_ptr<IMessageHandler> handler);
	void startAccept();

};

#endif // SERVER_H