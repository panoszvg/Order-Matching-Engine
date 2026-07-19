#include <gtest/gtest.h>
#include "tcp/FixOrderHandler.h"
#include "FakeSession.h"
#include "Book.h"
#include "Security.h"
#include "strategy/PriceTimePriorityStrategy.h"
#include "messages/IMessage.h"
#include <nlohmann/json.hpp>
#include <string>

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

std::string buildFix(const std::string& symbol, int side, double qty, double price) {
    const std::string soh(1, SOH);
    return "8=FIX.4.2" + soh
         + "35=D"      + soh
         + "55=" + symbol + soh
         + "54=" + std::to_string(side)  + soh
         + "38=" + std::to_string(qty)   + soh
         + "44=" + std::to_string(price) + soh
         + "10=000"    + soh;
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

    EXPECT_DOUBLE_EQ(books["TST"]->getBuyOrders().total_quantity, 5.0);
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
