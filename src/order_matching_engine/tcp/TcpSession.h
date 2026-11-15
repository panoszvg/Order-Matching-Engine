#ifndef TCPSESSION_H
#define TCPSESSION_H

#include <boost/asio.hpp>
#include <deque>
#include <memory>
#include <iostream>
#include <string>
#include "IMessageHandler.h"
#include "ISession.h"

class TcpSession : public ISession, public std::enable_shared_from_this<TcpSession> {
private:
	boost::asio::ip::tcp::socket socket_;
	// Room for the longest message plus its '\n'. When the buffer is full and has no '\n',
	// async_read_until fails with error::not_found.
	boost::asio::streambuf buffer_{kMaxMessageSize + 1};
	std::shared_ptr<IMessageHandler> handler_;
	// Replies waiting to be written. The front one is being written; it stays here until its
	// write completes, because async_write only points at the data. A deque doesn't move
	// existing elements on push_back, so that pointer stays valid.
	std::deque<std::string> writeQueue_;
	void read();
	void write();

public:
	// Longest message accepted, not counting the '\n'. A client that sends more without
	// a newline is disconnected, so it can't make the read buffer grow without limit.
	static constexpr std::size_t kMaxMessageSize = 64 * 1024;

	TcpSession(boost::asio::ip::tcp::socket socket, std::shared_ptr<IMessageHandler> handler)
		: socket_(std::move(socket)), handler_(std::move(handler)) {}

	void start();
	void send(const std::string& message) override;
};

#endif // TCPSESSION_H
