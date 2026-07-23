#include <gtest/gtest.h>
#include "messages/SimpleMessage/SimpleMessage.h"
#include <string>

namespace {

// Build a well-formed Simple message from individual fields.
std::string buildMsg(const std::string& symbol, int side, double qty, double price) {
    const std::string soh(1, SOH);
    return "55=" + symbol + soh
         + "54=" + std::to_string(side) + soh
         + "38=" + std::to_string(qty)  + soh
         + "44=" + std::to_string(price) + soh;
}

} // namespace

TEST(SimpleMessage, ParsesSymbol) {
    SimpleMessage msg;
    msg.populate(buildMsg("BTC", 1, 5.0, 100.0));
    Order order = msg.makeOrder();
    EXPECT_EQ(order.security, "BTC");
}

TEST(SimpleMessage, ParsesBuySide) {
    SimpleMessage msg;
    msg.populate(buildMsg("BTC", 1, 5.0, 100.0));
    Order order = msg.makeOrder();
    EXPECT_EQ(order.type, BUY);
}

TEST(SimpleMessage, ParsesSellSide) {
    SimpleMessage msg;
    msg.populate(buildMsg("BTC", 2, 5.0, 100.0));
    Order order = msg.makeOrder();
    EXPECT_EQ(order.type, SELL);
}

TEST(SimpleMessage, ParsesQuantity) {
    SimpleMessage msg;
    msg.populate(buildMsg("BTC", 1, 7.5, 100.0));
    Order order = msg.makeOrder();
    EXPECT_DOUBLE_EQ(order.quantity, 7.5);
}

TEST(SimpleMessage, ParsesPrice) {
    SimpleMessage msg;
    msg.populate(buildMsg("ETH", 2, 3.0, 200.5));
    Order order = msg.makeOrder();
    EXPECT_DOUBLE_EQ(order.price, 200.5);
}

TEST(SimpleMessage, FieldsInAnyOrderAreParsed) {
    // Rearrange: price before quantity before side before symbol
    const std::string soh(1, SOH);
    std::string scrambled = "44=99.0" + soh + "38=4.0" + soh + "54=2" + soh + "55=ADA" + soh;

    SimpleMessage msg;
    msg.populate(scrambled);
    Order order = msg.makeOrder();

    EXPECT_EQ(order.security, "ADA");
    EXPECT_EQ(order.type, SELL);
    EXPECT_DOUBLE_EQ(order.quantity, 4.0);
    EXPECT_DOUBLE_EQ(order.price, 99.0);
}

TEST(SimpleMessage, MissingSideField_Throws) {
    // tag 54 absent → stoi("") throws std::invalid_argument
    const std::string soh(1, SOH);
    std::string msg_str = "55=BTC" + soh + "38=5.0" + soh + "44=100.0" + soh;

    SimpleMessage msg;
    msg.populate(msg_str);
    EXPECT_THROW(msg.makeOrder(), std::invalid_argument);
}

TEST(SimpleMessage, MissingQuantityField_Throws) {
    // tag 38 absent → stod("") throws std::invalid_argument
    const std::string soh(1, SOH);
    std::string msg_str = "55=BTC" + soh + "54=1" + soh + "44=100.0" + soh;

    SimpleMessage msg;
    msg.populate(msg_str);
    EXPECT_THROW(msg.makeOrder(), std::invalid_argument);
}

TEST(SimpleMessage, MissingPriceField_Throws) {
    // tag 44 absent → stod("") throws std::invalid_argument
    const std::string soh(1, SOH);
    std::string msg_str = "55=BTC" + soh + "54=1" + soh + "38=5.0" + soh;

    SimpleMessage msg;
    msg.populate(msg_str);
    EXPECT_THROW(msg.makeOrder(), std::invalid_argument);
}

TEST(SimpleMessage, EmptyMessageBody_Throws) {
    SimpleMessage msg;
    msg.populate("");
    EXPECT_THROW(msg.makeOrder(), std::invalid_argument);
}

TEST(SimpleMessage, InvalidNonNumericTag_IsSkipped) {
    const std::string soh(1, SOH);
    // "abc=garbage" has a non-numeric tag — should be silently skipped
    std::string raw = "abc=garbage" + soh + buildMsg("BTC", 1, 5.0, 100.0);

    SimpleMessage msg;
    EXPECT_NO_THROW(msg.populate(raw));
    // Core fields still parsed correctly
    Order order = msg.makeOrder();
    EXPECT_EQ(order.security, "BTC");
}

TEST(SimpleMessage, UnknownTagsAreIgnored) {
    const std::string soh(1, SOH);
    // tag 999 is not a known field — should be silently ignored
    std::string msg_str = "999=garbage" + soh + buildMsg("BTC", 1, 2.0, 50.0);

    SimpleMessage msg;
    msg.populate(msg_str);
    Order order = msg.makeOrder();
    EXPECT_EQ(order.security, "BTC");
    EXPECT_EQ(order.type, BUY);
}

// ── isValid ──────────────────────────────────────────────────────────────────

TEST(SimpleMessage, IsValid_WellFormedMessage_ReturnsTrue) {
    SimpleMessage msg;
    msg.populate(buildMsg("BTC", 1, 5.0, 100.0));
    EXPECT_TRUE(msg.isValid());
}

TEST(SimpleMessage, IsValid_MissingSideField_ReturnsFalse) {
    const std::string soh(1, SOH);
    std::string msg_str = "55=BTC" + soh + "38=5.0" + soh + "44=100.0" + soh;
    SimpleMessage msg;
    msg.populate(msg_str);
    EXPECT_FALSE(msg.isValid());
}

TEST(SimpleMessage, IsValid_EmptyMessage_ReturnsFalse) {
    SimpleMessage msg;
    msg.populate("");
    EXPECT_FALSE(msg.isValid());
}
