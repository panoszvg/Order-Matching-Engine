#ifndef TCPSESSION_H
#define TCPSESSION_H

#include <boost/asio.hpp>
#include <memory>
#include <iostream>
#include "IMessageHandler.h"

using boost::asio::ip::tcp;

class TcpSession : public std::enable_shared_from_this<TcpSession> {
private:
	tcp::socket socket_;
	boost::asio::streambuf buffer_;
	std::shared_ptr<IMessageHandler> handler_;
	void read();

public:
	TcpSession(tcp::socket socket, std::shared_ptr<IMessageHandler> handler)
		: socket_(std::move(socket)), handler_(std::move(handler)) {}

	void start();

};

#endif // TCPSESSION_H
