#include <gtest/gtest.h>
#include "tcp/FixOrderHandler.h"
#include "FakeSession.h"
#include "TestHelpers.h"
#include "Book.h"
#include "Security.h"
#include "strategy/PriceTimePriorityStrategy.h"
#include "messages/IMessage.h"
#include <nlohmann/json.hpp>
#include <string>
#include <cstdio>

using json = nlohmann::json;

namespace {

std::unordered_map<std::string, std::unique_ptr<Book>> makeBooks() {
    std::unordered_map<std::string, std::unique_ptr<Book>> books;
    books["TST"] = std::make_unique<Book>(
        std::make_unique<PriceTimePriorityStrategy>(),
        std::make_unique<Security>("TST", 0.1, 0.5)
    );
    return books;
}

std::string computeChecksum(const std::string& body) {
    unsigned int sum = 0;
    for (unsigned char c : body) sum += c;
    char buf[4];
    std::snprintf(buf, sizeof(buf), "%03u", sum % 256);
    return buf;
}

// FIX 4.4 New Order Single (35=D), limit order, with correct BodyLength and CheckSum.
std::string buildFix(const std::string& symbol, int side, double qty, double price) {
    const std::string soh(1, SOH);
    std::string body = "35=D"      + soh
                     + "49=CLIENT" + soh
                     + "56=ENGINE" + soh
                     + "34=1"      + soh
                     + "52=20251105-12:00:00.000" + soh
                     + "11=ORD1"   + soh
                     + "55=" + symbol + soh
                     + "54=" + std::to_string(side)  + soh
                     + "38=" + std::to_string(qty)   + soh
                     + "40=2"      + soh
                     + "44=" + std::to_string(price) + soh
                     + "60=20251105-12:00:00.000" + soh;
    std::string msg = "8=FIX.4.4" + soh + "9=" + std::to_string(body.size()) + soh + body;
    return msg + "10=" + computeChecksum(msg) + soh;
}

} // namespace

TEST(FixOrderHandler, ValidOrder_SendsOkResponse) {
    auto books = makeBooks();
    FixOrderHandler handler(books);
    FakeSession session;

    handler.handle(buildFix("TST", 1, 5.0, 100.0), session);

    ASSERT_EQ(session.sent.size(), 1u);
    auto response = json::parse(session.sent[0]);
    EXPECT_EQ(response["status"], "ok");
}

TEST(FixOrderHandler, ValidOrder_InsertsIntoBook) {
    auto books = makeBooks();
    FixOrderHandler handler(books);
    FakeSession session;

    handler.handle(buildFix("TST", 1, 5.0, 100.0), session);

    EXPECT_DOUBLE_EQ(restingBuyQuantity(*books["TST"]), 5.0);
}

TEST(FixOrderHandler, UnknownSecurity_SendsErrorResponse) {
    auto books = makeBooks();
    FixOrderHandler handler(books);
    FakeSession session;

    handler.handle(buildFix("NOPE", 1, 5.0, 100.0), session);

    ASSERT_EQ(session.sent.size(), 1u);
    auto response = json::parse(session.sent[0]);
    EXPECT_EQ(response["status"], "error");
    EXPECT_EQ(response["message"], "Unknown security in FIX order");
}

TEST(FixOrderHandler, InvalidTickAlignment_SendsErrorResponse) {
    auto books = makeBooks();
    FixOrderHandler handler(books);
    FakeSession session;

    handler.handle(buildFix("TST", 1, 5.0, 100.05), session);

    ASSERT_EQ(session.sent.size(), 1u);
    auto response = json::parse(session.sent[0]);
    EXPECT_EQ(response["status"], "error");
}

TEST(FixOrderHandler, MalformedMessage_SendsErrorResponse) {
    auto books = makeBooks();
    FixOrderHandler handler(books);
    FakeSession session;

    handler.handle("garbage not fix", session);

    ASSERT_EQ(session.sent.size(), 1u);
    auto response = json::parse(session.sent[0]);
    EXPECT_EQ(response["status"], "error");
}

TEST(FixOrderHandler, InvalidChecksum_RejectedBeforeReachingBook) {
    auto books = makeBooks();
    FixOrderHandler handler(books);
    FakeSession session;

    std::string badChecksum = buildFix("TST", 1, 5.0, 100.0);
    auto pos = badChecksum.rfind("10=") + 3;
    char wrong[4];
    std::snprintf(wrong, sizeof(wrong), "%03d", (std::stoi(badChecksum.substr(pos, 3)) + 1) % 256);
    badChecksum.replace(pos, 3, wrong); // wrong checksum

    handler.handle(badChecksum, session);

    ASSERT_EQ(session.sent.size(), 1u);
    auto response = json::parse(session.sent[0]);
    EXPECT_EQ(response["status"], "error");
    EXPECT_EQ(response["message"], "Invalid FIX message");
    EXPECT_DOUBLE_EQ(restingBuyQuantity(*books["TST"]), 0.0);
}
