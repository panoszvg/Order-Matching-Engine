#ifndef TCPSESSION_H
#define TCPSESSION_H

#include <boost/asio.hpp>
#include <array>
#include <deque>
#include <memory>
#include <string>
#include "IFramer.h"
#include "IMessageHandler.h"
#include "ISession.h"

class TcpSession : public ISession, public std::enable_shared_from_this<TcpSession> {
private:
	boost::asio::ip::tcp::socket socket_;
	std::shared_ptr<IMessageHandler> handler_;
	std::shared_ptr<const IFramer> framer_;

	std::array<char, 4096> readBuffer_;
	std::string pending_; // bytes received but not yet part of a whole message

	// Replies waiting to be written. The front one is being written; it stays here until its
	// write completes, because async_write only points at the data. A deque doesn't move
	// existing elements on push_back, so that pointer stays valid.
	std::deque<std::string> writeQueue_;

	// After a framing error the stream can't be trusted: once the error reply is written the
	// session stops sending, and it ignores any further input until the client disconnects.
	bool framingFailed_ = false;

	void read();
	void handlePending();
	void handleMessage(const std::string& message);
	void write();

public:
	TcpSession(boost::asio::ip::tcp::socket socket, std::shared_ptr<IMessageHandler> handler,
		std::shared_ptr<const IFramer> framer)
		: socket_(std::move(socket)), handler_(std::move(handler)), framer_(std::move(framer)) {}

	void start();
	void send(const std::string& message) override;
};

#endif // TCPSESSION_H
