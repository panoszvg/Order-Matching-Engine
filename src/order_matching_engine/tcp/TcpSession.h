#ifndef TCPSESSION_H
#define TCPSESSION_H

#include <boost/asio.hpp>
#include <deque>
#include <memory>
#include <iostream>
#include <string>
#include "IMessageHandler.h"
#include "ISession.h"

using boost::asio::ip::tcp;

class TcpSession : public ISession, public std::enable_shared_from_this<TcpSession> {
private:
	tcp::socket socket_;
	boost::asio::streambuf buffer_;
	std::shared_ptr<IMessageHandler> handler_;
	// Replies waiting to be written. The front one is being written; it stays here until its
	// write completes, because async_write only points at the data. A deque doesn't move
	// existing elements on push_back, so that pointer stays valid.
	std::deque<std::string> writeQueue_;
	void read();
	void write();

public:
	TcpSession(tcp::socket socket, std::shared_ptr<IMessageHandler> handler)
		: socket_(std::move(socket)), handler_(std::move(handler)) {}

	void start();
	void send(const std::string& message) override;
};

#endif // TCPSESSION_H
