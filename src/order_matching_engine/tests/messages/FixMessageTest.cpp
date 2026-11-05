#include <gtest/gtest.h>
#include "messages/FixMessage/FixMessage.h"
#include <algorithm>
#include <string>
#include <utility>
#include <vector>
#include <cstdio>

namespace {

using Fields = std::vector<std::pair<int, std::string>>;

const std::string soh(1, SOH);
const std::string TS = "20251105-12:00:00.000";

std::string computeChecksum(const std::string& body) {
    unsigned int sum = 0;
    for (unsigned char c : body) sum += c;
    char buf[4];
    std::snprintf(buf, sizeof(buf), "%03u", sum % 256);
    return buf;
}

// Header (after BodyLength) + body of a FIX 4.4 New Order Single (35=D), limit order.
Fields newOrderFields(const std::string& symbol = "BTC", int side = 1, double qty = 5.0, double price = 100.0) {
    return {
        {35, "D"}, {49, "CLIENT"}, {56, "ENGINE"}, {34, "1"}, {52, TS},
        {11, "ORD1"}, {55, symbol}, {54, std::to_string(side)}, {38, std::to_string(qty)},
        {40, "2"}, {44, std::to_string(price)}, {60, TS},
    };
}

Fields without(Fields fields, int tag) {
    fields.erase(std::remove_if(fields.begin(), fields.end(),
                                [tag](const auto& f) { return f.first == tag; }),
                 fields.end());
    return fields;
}

Fields with(Fields fields, int tag, const std::string& value) {
    for (auto& f : fields) {
        if (f.first == tag) { f.second = value; return fields; }
    }
    fields.emplace_back(tag, value);
    return fields;
}

// Prepend BeginString + correct BodyLength, append correct CheckSum.
std::string frame(const Fields& fields, const std::string& beginString = "FIX.4.4") {
    std::string body;
    for (const auto& [tag, value] : fields) body += std::to_string(tag) + "=" + value + soh;
    std::string msg = "8=" + beginString + soh + "9=" + std::to_string(body.size()) + soh + body;
    return msg + "10=" + computeChecksum(msg) + soh;
}

std::string buildFix(const std::string& symbol, int side, double qty, double price) {
    return frame(newOrderFields(symbol, side, qty, price));
}

bool isValid(const std::string& raw) {
    FixMessage msg;
    msg.populate(raw);
    return msg.isValid();
}

} // namespace

TEST(FixMessage, ParsesSymbol) {
    FixMessage msg;
    msg.populate(buildFix("BTC", 1, 5.0, 100.0));
    Order order = msg.makeOrder();
    EXPECT_EQ(order.security, "BTC");
}

TEST(FixMessage, ParsesBuySide) {
    FixMessage msg;
    msg.populate(buildFix("BTC", 1, 5.0, 100.0));
    Order order = msg.makeOrder();
    EXPECT_EQ(order.type, BUY);
}

TEST(FixMessage, ParsesSellSide) {
    FixMessage msg;
    msg.populate(buildFix("ETH", 2, 3.0, 200.0));
    Order order = msg.makeOrder();
    EXPECT_EQ(order.type, SELL);
}

TEST(FixMessage, ParsesQuantity) {
    FixMessage msg;
    msg.populate(buildFix("BTC", 1, 7.5, 100.0));
    Order order = msg.makeOrder();
    EXPECT_DOUBLE_EQ(order.quantity, 7.5);
}

TEST(FixMessage, ParsesPrice) {
    FixMessage msg;
    msg.populate(buildFix("ADA", 2, 4.0, 55.5));
    Order order = msg.makeOrder();
    EXPECT_DOUBLE_EQ(order.price, 55.5);
}

TEST(FixMessage, GetValue_HeaderField) {
    FixMessage msg;
    msg.populate(buildFix("BTC", 1, 5.0, 100.0));
    // tag 8 (BeginString) lands in the header section
    EXPECT_EQ(msg.getValue(8), "FIX.4.4");
}

TEST(FixMessage, GetValue_BodyField) {
    FixMessage msg;
    msg.populate(buildFix("BTC", 1, 5.0, 100.0));
    // tag 55 (Symbol) lands in the body section (after 35=D)
    EXPECT_EQ(msg.getValue(55), "BTC");
}

TEST(FixMessage, GetValue_TrailerField) {
    FixMessage msg;
    msg.populate(buildFix("BTC", 1, 5.0, 100.0));
    // tag 10 (Checksum) lands in the trailer section and is not empty
    EXPECT_FALSE(msg.getValue(10).empty());
}

TEST(FixMessage, GetValue_MissingTag_ReturnsEmpty) {
    FixMessage msg;
    msg.populate(buildFix("BTC", 1, 5.0, 100.0));
    EXPECT_EQ(msg.getValue(999), "");
}

TEST(FixMessage, Tag35_TransitionsToBodySection) {
    FixMessage msg;
    msg.populate(buildFix("BTC", 1, 5.0, 100.0));
    // Fields after 35=D should be in body, not header
    EXPECT_EQ(msg.getValue(55), "BTC");  // body field accessible
    EXPECT_EQ(msg.getValue(8),  "FIX.4.4"); // header field still accessible
}

TEST(FixMessage, InvalidNonNumericTag_IsSkipped) {
    // "abc=garbage" has a non-numeric tag — should be silently skipped
    std::string raw = "abc=garbage" + soh + buildFix("BTC", 1, 5.0, 100.0);
    FixMessage msg;
    EXPECT_NO_THROW(msg.populate(raw));
    // Core fields still parsed correctly
    Order order = msg.makeOrder();
    EXPECT_EQ(order.security, "BTC");
}

TEST(FixMessage, MakeOrder_MissingSideField_Throws) {
    // tag 54 absent → stoi("") throws
    FixMessage msg;
    msg.populate(frame(without(newOrderFields(), 54)));
    EXPECT_THROW(msg.makeOrder(), std::exception);
}

TEST(FixMessage, MakeOrder_MissingQuantityField_Throws) {
    FixMessage msg;
    msg.populate(frame(without(newOrderFields(), 38)));
    EXPECT_THROW(msg.makeOrder(), std::exception);
}

TEST(FixMessage, MakeOrder_MissingPriceField_Throws) {
    FixMessage msg;
    msg.populate(frame(without(newOrderFields(), 44)));
    EXPECT_THROW(msg.makeOrder(), std::exception);
}

TEST(FixMessage, MakeOrder_MarketOrder_Throws) {
    // Valid FIX 4.4, but the engine only supports limit orders
    FixMessage msg;
    msg.populate(frame(without(with(newOrderFields(), 40, "1"), 44)));
    EXPECT_TRUE(msg.isValid());
    EXPECT_THROW(msg.makeOrder(), std::invalid_argument);
}

TEST(FixMessage, MakeOrder_UnsupportedSide_Throws) {
    FixMessage msg;
    msg.populate(frame(with(newOrderFields(), 54, "5"))); // 5 = Sell short
    EXPECT_THROW(msg.makeOrder(), std::invalid_argument);
}

TEST(FixMessage, EmptyMessage_GetValueReturnsEmpty) {
    FixMessage msg;
    msg.populate("");
    EXPECT_EQ(msg.getValue(55), "");
    EXPECT_EQ(msg.getValue(54), "");
}

// ── isValid ──────────────────────────────────────────────────────────────────

TEST(FixMessage, IsValid_WellFormedMessage_ReturnsTrue) {
    EXPECT_TRUE(isValid(buildFix("BTC", 1, 5.0, 100.0)));
}

TEST(FixMessage, IsValid_WrongChecksum_ReturnsFalse) {
    std::string raw = buildFix("BTC", 1, 5.0, 100.0);
    auto pos = raw.rfind("10=");
    char wrong[4];
    std::snprintf(wrong, sizeof(wrong), "%03d", (std::stoi(raw.substr(pos + 3, 3)) + 1) % 256);
    raw.replace(pos + 3, 3, wrong); // deliberately wrong checksum
    EXPECT_FALSE(isValid(raw));
}

TEST(FixMessage, IsValid_MissingChecksumField_ReturnsFalse) {
    std::string raw = buildFix("BTC", 1, 5.0, 100.0);
    raw.erase(raw.rfind("10=")); // no tag 10 at all
    EXPECT_FALSE(isValid(raw));
}

TEST(FixMessage, IsValid_EmptyMessage_ReturnsFalse) {
    EXPECT_FALSE(isValid(""));
}

TEST(FixMessage, IsValid_OtherFixVersion_ReturnsFalse) {
    EXPECT_FALSE(isValid(frame(newOrderFields(), "FIX.4.2")));
}

TEST(FixMessage, IsValid_WrongBodyLength_ReturnsFalse) {
    std::string raw = frame(newOrderFields());
    auto start = raw.find("9=") + 2;
    auto end = raw.find(SOH, start);
    std::string wrong = std::to_string(std::stoi(raw.substr(start, end - start)) + 1);
    raw.replace(start, end - start, wrong);
    // recompute checksum so only BodyLength is wrong
    auto pos = raw.rfind("10=");
    raw = raw.substr(0, pos) + "10=" + computeChecksum(raw.substr(0, pos)) + soh;
    EXPECT_FALSE(isValid(raw));
}

TEST(FixMessage, IsValid_MsgTypeNotThirdField_ReturnsFalse) {
    Fields fields = without(newOrderFields(), 35);
    fields.insert(fields.begin() + 1, {35, "D"});
    EXPECT_FALSE(isValid(frame(fields)));
}

TEST(FixMessage, IsValid_NotNewOrderSingle_ReturnsFalse) {
    EXPECT_FALSE(isValid(frame(with(newOrderFields(), 35, "8"))));
}

TEST(FixMessage, IsValid_MissingRequiredField_ReturnsFalse) {
    for (int tag : {49, 56, 34, 52, 11, 55, 54, 38, 40, 60}) {
        EXPECT_FALSE(isValid(frame(without(newOrderFields(), tag)))) << "missing tag " << tag;
    }
}

TEST(FixMessage, IsValid_LimitOrderWithoutPrice_ReturnsFalse) {
    EXPECT_FALSE(isValid(frame(without(newOrderFields(), 44))));
}

TEST(FixMessage, IsValid_MarketOrderWithoutPrice_ReturnsTrue) {
    EXPECT_TRUE(isValid(frame(without(with(newOrderFields(), 40, "1"), 44))));
}

TEST(FixMessage, IsValid_StopLimitWithoutStopPx_ReturnsFalse) {
    EXPECT_FALSE(isValid(frame(with(newOrderFields(), 40, "4"))));
    EXPECT_TRUE(isValid(frame(with(with(newOrderFields(), 40, "4"), 99, "101.0"))));
}
