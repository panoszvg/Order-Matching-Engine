#include <gtest/gtest.h>
#include "tcp/JsonOrderHandler.h"
#include "FakeSession.h"
#include "Book.h"
#include "Security.h"
#include "strategy/PriceTimePriorityStrategy.h"
#include <nlohmann/json.hpp>

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

std::string buildJson(const std::string& security, const std::string& type, double qty, double price) {
    json j = {
        {"security", security},
        {"type", type},
        {"quantity", qty},
        {"price", price}
    };
    return j.dump();
}

} // namespace

TEST(JsonOrderHandler, ValidOrder_SendsOkResponse) {
    auto books = makeBooks();
    JsonOrderHandler handler(books);
    FakeSession session;

    handler.handle(buildJson("TST", "BUY", 5.0, 100.0), session);

    ASSERT_EQ(session.sent.size(), 1u);
    auto response = json::parse(session.sent[0]);
    EXPECT_EQ(response["status"], "ok");
}

TEST(JsonOrderHandler, ValidOrder_InsertsIntoBook) {
    auto books = makeBooks();
    JsonOrderHandler handler(books);
    FakeSession session;

    handler.handle(buildJson("TST", "BUY", 5.0, 100.0), session);

    EXPECT_DOUBLE_EQ(books["TST"]->getBuyOrders().total_quantity, 5.0);
}

TEST(JsonOrderHandler, UnknownSecurity_SendsErrorResponse) {
    auto books = makeBooks();
    JsonOrderHandler handler(books);
    FakeSession session;

    handler.handle(buildJson("NOPE", "BUY", 5.0, 100.0), session);

    ASSERT_EQ(session.sent.size(), 1u);
    auto response = json::parse(session.sent[0]);
    EXPECT_EQ(response["status"], "error");
    EXPECT_EQ(response["message"], "Unknown security");
}

TEST(JsonOrderHandler, InvalidTickAlignment_SendsErrorResponse) {
    auto books = makeBooks();
    JsonOrderHandler handler(books);
    FakeSession session;

    // Security tick is 0.1; 100.05 does not align
    handler.handle(buildJson("TST", "BUY", 5.0, 100.05), session);

    ASSERT_EQ(session.sent.size(), 1u);
    auto response = json::parse(session.sent[0]);
    EXPECT_EQ(response["status"], "error");
}

TEST(JsonOrderHandler, MalformedJson_SendsErrorResponse) {
    auto books = makeBooks();
    JsonOrderHandler handler(books);
    FakeSession session;

    handler.handle("not json at all", session);

    ASSERT_EQ(session.sent.size(), 1u);
    auto response = json::parse(session.sent[0]);
    EXPECT_EQ(response["status"], "error");
}

TEST(JsonOrderHandler, MissingField_SendsErrorResponse) {
    auto books = makeBooks();
    JsonOrderHandler handler(books);
    FakeSession session;

    json j = {{"security", "TST"}, {"type", "BUY"}};  // missing price/quantity
    handler.handle(j.dump(), session);

    ASSERT_EQ(session.sent.size(), 1u);
    auto response = json::parse(session.sent[0]);
    EXPECT_EQ(response["status"], "error");
}
