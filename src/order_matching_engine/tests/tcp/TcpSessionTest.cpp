#include <gtest/gtest.h>
#include "tcp/TcpSession.h"
#include <boost/asio.hpp>
#include <chrono>
#include <functional>
#include <string>
#include <vector>

namespace asio = boost::asio;
using asio::ip::tcp;

namespace {

// Bigger than a localhost socket's send buffer, so a reply can't be written in one go:
// its async_write stays pending while the session carries on handling messages.
constexpr std::size_t kReplySize = 4 * 1024 * 1024;

// The reply to request "<n>" is one line: "<n>:" then kReplySize copies of one letter.
std::string expectedReply(const std::string& request) {
    int n = std::stoi(request);
    return request + ":" + std::string(kReplySize, static_cast<char>('A' + n % 26));
}

class LargeReplyHandler : public IMessageHandler {
public:
    void handle(const std::string& rawMessage, ISession& session) override {
        session.send(expectedReply(rawMessage));
    }
};

class EchoHandler : public IMessageHandler {
public:
    void handle(const std::string& rawMessage, ISession& session) override {
        session.send(rawMessage);
    }
};

// Connects a client to a TcpSession over localhost, with both on one io_context.
// Sends all requests in a single write, then collects reply lines until expectedLines
// have arrived, the connection fails, or the timeout passes.
std::vector<std::string> sendAndCollectReplies(const std::vector<std::string>& requests, std::size_t expectedLines,
                                               std::shared_ptr<IMessageHandler> handler) {
    asio::io_context io;
    tcp::acceptor acceptor(io, tcp::endpoint(asio::ip::address_v4::loopback(), 0));
    tcp::socket client(io);
    client.connect(acceptor.local_endpoint());
    std::make_shared<TcpSession>(acceptor.accept(), handler)->start();

    std::string out;
    for (const auto& request : requests) out += request + '\n';
    asio::async_write(client, asio::buffer(out), [](boost::system::error_code, std::size_t) {});

    std::vector<std::string> lines;
    asio::streambuf in;
    std::function<void()> readLine = [&]() {
        asio::async_read_until(client, in, '\n', [&](boost::system::error_code ec, std::size_t) {
            if (ec) return;
            std::istream input(&in);
            std::string line;
            std::getline(input, line);
            lines.push_back(std::move(line));
            if (lines.size() < expectedLines) readLine();
            else io.stop();
        });
    };
    readLine();

    io.run_for(std::chrono::seconds(5));
    return lines;
}

} // namespace

// Also catches send() writing from a freed buffer, but only when built with -fsanitize=address.
TEST(TcpSession, PipelinedRequests_RepliesArriveWholeAndInOrder) {
    std::vector<std::string> requests = {"0", "1", "2", "3", "4"};
    auto lines = sendAndCollectReplies(requests, requests.size(), std::make_shared<LargeReplyHandler>());

    ASSERT_EQ(lines.size(), requests.size());
    for (std::size_t i = 0; i < requests.size(); ++i) {
        std::string expected = expectedReply(requests[i]);
        // Checked by size and start, so a failure doesn't print 4 MB strings
        EXPECT_EQ(lines[i].size(), expected.size()) << "reply " << i;
        EXPECT_EQ(lines[i].substr(0, 16), expected.substr(0, 16)) << "reply " << i;
    }
}

TEST(TcpSession, CrLfLineEnding_CrIsRemoved) {
    auto lines = sendAndCollectReplies({"abc\r"}, 1, std::make_shared<EchoHandler>());

    ASSERT_EQ(lines.size(), 1u);
    EXPECT_EQ(lines[0], "abc");
}

TEST(TcpSession, MessageOverSizeLimit_IsNotHandled) {
    std::string tooLong(TcpSession::kMaxMessageSize + 1, 'x');
    auto lines = sendAndCollectReplies({tooLong}, 1, std::make_shared<EchoHandler>());

    EXPECT_TRUE(lines.empty());
}
