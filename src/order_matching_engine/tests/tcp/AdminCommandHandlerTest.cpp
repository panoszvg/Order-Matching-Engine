#include <gtest/gtest.h>
#include "tcp/AdminCommandHandler.h"
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

} // namespace

TEST(AdminCommandHandler, GetSnapshot_KnownSymbol_ReturnsOk) {
    auto books = makeBooks();
    AdminCommandHandler handler(books);
    FakeSession session;

    json cmd = {{"type", "GET_SNAPSHOT"}, {"symbol", "TST"}};
    handler.handle(cmd.dump(), session);

    ASSERT_EQ(session.sent.size(), 1u);
    auto response = json::parse(session.sent[0]);
    EXPECT_EQ(response["status"], "ok");
    EXPECT_EQ(response["book"]["symbol"], "TST");
}

TEST(AdminCommandHandler, GetSnapshot_UnknownSymbol_ReturnsError) {
    auto books = makeBooks();
    AdminCommandHandler handler(books);
    FakeSession session;

    json cmd = {{"type", "GET_SNAPSHOT"}, {"symbol", "NOPE"}};
    handler.handle(cmd.dump(), session);

    ASSERT_EQ(session.sent.size(), 1u);
    auto response = json::parse(session.sent[0]);
    EXPECT_EQ(response["status"], "error");
    EXPECT_EQ(response["message"], "Unknown symbol");
}

TEST(AdminCommandHandler, ExportSnapshot_UnknownSymbol_ReturnsError) {
    auto books = makeBooks();
    AdminCommandHandler handler(books);
    FakeSession session;

    json cmd = {{"type", "EXPORT_SNAPSHOT"}, {"symbol", "NOPE"}};
    handler.handle(cmd.dump(), session);

    ASSERT_EQ(session.sent.size(), 1u);
    auto response = json::parse(session.sent[0]);
    EXPECT_EQ(response["status"], "error");
    EXPECT_EQ(response["message"], "Unknown symbol");
}

TEST(AdminCommandHandler, UnknownCommandType_ReturnsError) {
    auto books = makeBooks();
    AdminCommandHandler handler(books);
    FakeSession session;

    json cmd = {{"type", "DO_A_BARREL_ROLL"}};
    handler.handle(cmd.dump(), session);

    ASSERT_EQ(session.sent.size(), 1u);
    auto response = json::parse(session.sent[0]);
    EXPECT_EQ(response["status"], "error");
    EXPECT_EQ(response["message"], "Unknown command");
}

TEST(AdminCommandHandler, MalformedJson_ReturnsError) {
    auto books = makeBooks();
    AdminCommandHandler handler(books);
    FakeSession session;

    handler.handle("not json", session);

    ASSERT_EQ(session.sent.size(), 1u);
    auto response = json::parse(session.sent[0]);
    EXPECT_EQ(response["status"], "error");
}
