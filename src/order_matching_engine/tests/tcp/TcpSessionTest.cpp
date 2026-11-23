#include <gtest/gtest.h>
#include "tcp/TcpSession.h"
#include "tcp/FixFramer.h"
#include "tcp/LineFramer.h"
#include "messages/IMessage.h"
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
// Sends all requests in a single write, each followed by `terminator`, then collects reply lines
// until expectedLines have arrived, the connection ends, or the timeout passes.
std::vector<std::string> sendAndCollectReplies(const std::vector<std::string>& requests, std::size_t expectedLines,
                                               std::shared_ptr<IMessageHandler> handler,
                                               std::shared_ptr<const IFramer> framer = std::make_shared<LineFramer>(),
                                               const std::string& terminator = "\n") {
    asio::io_context io;
    tcp::acceptor acceptor(io, tcp::endpoint(asio::ip::address_v4::loopback(), 0));
    tcp::socket client(io);
    client.connect(acceptor.local_endpoint());
    std::make_shared<TcpSession>(acceptor.accept(), handler, framer)->start();

    std::string out;
    for (const auto& request : requests) out += request + terminator;
    asio::async_write(client, asio::buffer(out), [](boost::system::error_code, std::size_t) {});

    std::vector<std::string> lines;
    asio::streambuf in;
    std::function<void()> readLine = [&]() {
        asio::async_read_until(client, in, '\n', [&](boost::system::error_code ec, std::size_t) {
            if (ec) { // e.g. the session ended the connection
                io.stop();
                return;
            }
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

TEST(TcpSession, MessageOverSizeLimit_IsReportedAndTheConnectionEnds) {
    std::string tooLong(LineFramer::kMaxMessageSize + 1, 'x');
    // Asks for one more line than should come, so an echo of "after" would show up
    auto lines = sendAndCollectReplies({"before", tooLong, "after"}, 3, std::make_shared<EchoHandler>());

    ASSERT_EQ(lines.size(), 2u);
    EXPECT_EQ(lines[0], "before");
    EXPECT_NE(lines[1].find("Framing error"), std::string::npos) << lines[1].substr(0, 80);
}

namespace {

class ThrowingHandler : public IMessageHandler {
public:
    void handle(const std::string& rawMessage, ISession& session) override {
        if (rawMessage == "throw") throw 42; // not even a std::exception
        session.send(rawMessage);
    }
};

} // namespace

TEST(TcpSession, HandlerException_IsReportedAndTheSessionCarriesOn) {
    auto lines = sendAndCollectReplies({"throw", "after"}, 2, std::make_shared<ThrowingHandler>());

    ASSERT_EQ(lines.size(), 2u);
    EXPECT_NE(lines[0].find("Internal error"), std::string::npos) << lines[0];
    EXPECT_EQ(lines[1], "after");
}

namespace {

// A FIX-shaped frame: BodyLength counts `body`; FixFramer doesn't check the CheckSum digits.
std::string fixFrame(const std::string& body) {
    const std::string soh(1, SOH);
    return "8=FIX.4.4" + soh + "9=" + std::to_string(body.size()) + soh + body + "10=000" + soh;
}

} // namespace

TEST(TcpSession, FixFramer_BackToBackMessagesWithoutNewlines_AreSplitByBodyLength) {
    const std::string soh(1, SOH);
    std::vector<std::string> requests = {
        fixFrame("35=D" + soh + "55=AAA" + soh),
        fixFrame("35=D" + soh + "58=a\nb" + soh), // a '\n' inside a value doesn't end the message
        fixFrame("35=D" + soh + "55=CCC" + soh),
    };
    auto lines = sendAndCollectReplies(requests, 4, std::make_shared<EchoHandler>(), std::make_shared<FixFramer>(), "");

    // Each message is echoed whole; the second echo holds a '\n', so it arrives as two reply lines
    ASSERT_EQ(lines.size(), 4u);
    EXPECT_EQ(lines[0], requests[0]);
    EXPECT_EQ(lines[1] + "\n" + lines[2], requests[1]);
    EXPECT_EQ(lines[3], requests[2]);
}

TEST(TcpSession, FixFramer_NonFixInput_IsReportedAndTheConnectionEnds) {
    const std::string soh(1, SOH);
    const std::string good = fixFrame("35=D" + soh + "55=AAA" + soh);
    // JSON sent to the FIX port by mistake. Messages before it are handled; nothing after it is.
    auto lines = sendAndCollectReplies({good, "{\"json\":\"on the wrong port\"}", good}, 3,
                                       std::make_shared<EchoHandler>(), std::make_shared<FixFramer>(), "");

    ASSERT_EQ(lines.size(), 2u);
    EXPECT_EQ(lines[0], good);
    EXPECT_NE(lines[1].find("Framing error"), std::string::npos) << lines[1];
}

TEST(TcpSession, FixFramer_WrongBodyLength_IsReportedAndTheConnectionEnds) {
    const std::string soh(1, SOH);
    const std::string good = fixFrame("35=D" + soh + "55=AAA" + soh);
    std::string wrongLength = fixFrame("35=D" + soh + "55=BAD" + soh);
    wrongLength.replace(wrongLength.find("9=") + 2, 2, "20"); // too long: the framer reads into the next message and finds no trailer where BodyLength says
    auto lines = sendAndCollectReplies({wrongLength, good}, 2, std::make_shared<EchoHandler>(),
                                       std::make_shared<FixFramer>(), "");

    ASSERT_EQ(lines.size(), 1u);
    EXPECT_NE(lines[0].find("Framing error"), std::string::npos) << lines[0];
}
