#include <gtest/gtest.h>
#include "messages/FixMessage/FixMessage.h"
#include <string>
#include <cstdio>

namespace {

std::string computeChecksum(const std::string& body) {
    unsigned int sum = 0;
    for (unsigned char c : body) sum += c;
    char buf[4];
    std::snprintf(buf, sizeof(buf), "%03u", sum % 256);
    return buf;
}

// Build a minimal, checksum-valid FIX 4.2 New Order Single (35=D).
std::string buildFix(const std::string& symbol, int side, double qty, double price) {
    const std::string soh(1, SOH);
    std::string body = "8=FIX.4.2" + soh
                      + "35=D"      + soh
                      + "55=" + symbol + soh
                      + "54=" + std::to_string(side)  + soh
                      + "38=" + std::to_string(qty)   + soh
                      + "44=" + std::to_string(price) + soh;
    return body + "10=" + computeChecksum(body) + soh;
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
    EXPECT_EQ(msg.getValue(8), "FIX.4.2");
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
    EXPECT_EQ(msg.getValue(8),  "FIX.4.2"); // header field still accessible
}

TEST(FixMessage, InvalidNonNumericTag_IsSkipped) {
    const std::string soh(1, SOH);
    // "abc=garbage" has a non-numeric tag — should be silently skipped
    std::string raw = "abc=garbage" + soh + buildFix("BTC", 1, 5.0, 100.0);
    FixMessage msg;
    EXPECT_NO_THROW(msg.populate(raw));
    // Core fields still parsed correctly
    Order order = msg.makeOrder();
    EXPECT_EQ(order.security, "BTC");
}

TEST(FixMessage, MakeOrder_MissingSideField_Throws) {
    const std::string soh(1, SOH);
    // tag 54 absent → stoi("") throws
    std::string raw = "8=FIX.4.2" + soh + "35=D" + soh
                    + "55=BTC"    + soh
                    + "38=5.0"   + soh
                    + "44=100.0" + soh
                    + "10=000"   + soh;
    FixMessage msg;
    msg.populate(raw);
    EXPECT_THROW(msg.makeOrder(), std::exception);
}

TEST(FixMessage, MakeOrder_MissingQuantityField_Throws) {
    const std::string soh(1, SOH);
    std::string raw = "8=FIX.4.2" + soh + "35=D" + soh
                    + "55=BTC"    + soh
                    + "54=1"      + soh
                    + "44=100.0"  + soh
                    + "10=000"    + soh;
    FixMessage msg;
    msg.populate(raw);
    EXPECT_THROW(msg.makeOrder(), std::exception);
}

TEST(FixMessage, MakeOrder_MissingPriceField_Throws) {
    const std::string soh(1, SOH);
    std::string raw = "8=FIX.4.2" + soh + "35=D" + soh
                    + "55=BTC"    + soh
                    + "54=1"      + soh
                    + "38=5.0"    + soh
                    + "10=000"    + soh;
    FixMessage msg;
    msg.populate(raw);
    EXPECT_THROW(msg.makeOrder(), std::exception);
}

TEST(FixMessage, EmptyMessage_GetValueReturnsEmpty) {
    FixMessage msg;
    msg.populate("");
    EXPECT_EQ(msg.getValue(55), "");
    EXPECT_EQ(msg.getValue(54), "");
}

// ── isValid ──────────────────────────────────────────────────────────────────

TEST(FixMessage, IsValid_WellFormedMessage_ReturnsTrue) {
    FixMessage msg;
    msg.populate(buildFix("BTC", 1, 5.0, 100.0));
    EXPECT_TRUE(msg.isValid());
}

TEST(FixMessage, IsValid_WrongChecksum_ReturnsFalse) {
    const std::string soh(1, SOH);
    std::string raw = "8=FIX.4.2" + soh + "35=D" + soh
                     + "55=BTC"    + soh
                     + "54=1"      + soh
                     + "38=5.0"    + soh
                     + "44=100.0"  + soh
                     + "10=000"    + soh; // deliberately wrong checksum
    FixMessage msg;
    msg.populate(raw);
    EXPECT_FALSE(msg.isValid());
}

TEST(FixMessage, IsValid_MissingChecksumField_ReturnsFalse) {
    const std::string soh(1, SOH);
    std::string raw = "8=FIX.4.2" + soh + "35=D" + soh
                     + "55=BTC"    + soh
                     + "54=1"      + soh
                     + "38=5.0"    + soh
                     + "44=100.0"  + soh; // no tag 10 at all
    FixMessage msg;
    msg.populate(raw);
    EXPECT_FALSE(msg.isValid());
}

TEST(FixMessage, IsValid_MissingRequiredField_ReturnsFalse) {
    const std::string soh(1, SOH);
    // 44 (Price) omitted from the body used for the checksum
    std::string body = "8=FIX.4.2" + soh + "35=D" + soh
                      + "55=BTC"    + soh
                      + "54=1"      + soh
                      + "38=5.0"    + soh;
    unsigned int sum = 0;
    for (unsigned char c : body) sum += c;
    char buf[4];
    std::snprintf(buf, sizeof(buf), "%03u", sum % 256);
    std::string raw = body + "10=" + buf + soh;

    FixMessage msg;
    msg.populate(raw);
    EXPECT_FALSE(msg.isValid());
}

TEST(FixMessage, IsValid_EmptyMessage_ReturnsFalse) {
    FixMessage msg;
    msg.populate("");
    EXPECT_FALSE(msg.isValid());
}
