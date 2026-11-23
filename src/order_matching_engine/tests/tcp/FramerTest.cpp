#include <gtest/gtest.h>
#include "tcp/FixFramer.h"
#include "tcp/LineFramer.h"
#include "messages/IMessage.h"
#include <string>

namespace {

const std::string soh(1, SOH);

// A well-formed frame: BodyLength counts `body` exactly; the CheckSum digits aren't checked by the framer.
std::string fixFrame(const std::string& body) {
    return "8=FIX.4.4" + soh + "9=" + std::to_string(body.size()) + soh + body + "10=123" + soh;
}

const std::string body = "35=D" + soh + "55=BTC" + soh;

} // namespace

// ── FixFramer ────────────────────────────────────────────────────────────────

TEST(FixFramer, WholeMessage_IsComplete) {
    const std::string msg = fixFrame(body);
    Frame f = FixFramer().next(msg);
    ASSERT_EQ(f.status, Frame::Status::Complete);
    EXPECT_EQ(f.consumed, msg.size());
    EXPECT_EQ(f.message, msg);
}

TEST(FixFramer, EveryPrefixOfAMessage_IsIncomplete) {
    const std::string msg = fixFrame(body);
    for (std::size_t n = 0; n < msg.size(); ++n) {
        EXPECT_EQ(FixFramer().next(std::string_view(msg).substr(0, n)).status, Frame::Status::Incomplete) << "n=" << n;
    }
}

TEST(FixFramer, BackToBackMessages_AreSplitByBodyLength) {
    const std::string first = fixFrame(body);
    const std::string second = fixFrame("35=D" + soh + "55=ETH" + soh);
    Frame f = FixFramer().next(first + second);
    ASSERT_EQ(f.status, Frame::Status::Complete);
    EXPECT_EQ(f.message, first);
    EXPECT_EQ(FixFramer().next(std::string_view(first + second).substr(f.consumed)).message, second);
}

TEST(FixFramer, NewlineInsideAValue_DoesNotEndTheMessage) {
    const std::string msg = fixFrame("35=D" + soh + "58=line one\nline two" + soh);
    Frame f = FixFramer().next(msg);
    ASSERT_EQ(f.status, Frame::Status::Complete);
    EXPECT_EQ(f.message, msg);
}

TEST(FixFramer, WrongBeginString_IsInvalid) {
    EXPECT_EQ(FixFramer().next("8=FIX.4.2" + soh + "9=5" + soh).status, Frame::Status::Invalid);
    EXPECT_EQ(FixFramer().next("{\"type\":").status, Frame::Status::Invalid);
    EXPECT_EQ(FixFramer().next("\n" + fixFrame(body)).status, Frame::Status::Invalid); // FIX has no line endings
}

TEST(FixFramer, BadBodyLength_IsInvalid) {
    const std::string start = "8=FIX.4.4" + soh + "9=";
    EXPECT_EQ(FixFramer().next(start + "1x" + soh).status, Frame::Status::Invalid);
    EXPECT_EQ(FixFramer().next(start + soh).status, Frame::Status::Invalid);
    EXPECT_EQ(FixFramer().next(start + "-5" + soh).status, Frame::Status::Invalid);
    EXPECT_EQ(FixFramer().next(start + "012" + soh).status, Frame::Status::Invalid);
}

TEST(FixFramer, EndlessZerosInBodyLength_AreInvalidAtOnce) {
    // Zeros keep the value at 0, so without this check the session would buffer them forever
    EXPECT_EQ(FixFramer().next("8=FIX.4.4" + soh + "9=0").status, Frame::Status::Invalid);
    EXPECT_EQ(FixFramer().next("8=FIX.4.4" + soh + "9=" + std::string(100000, '0')).status, Frame::Status::Invalid);
}

TEST(FixFramer, BodyLengthOverLimit_IsInvalidBeforeTheBodyArrives) {
    const std::string start = "8=FIX.4.4" + soh + "9=" + std::to_string(FixFramer::kMaxBodyLength + 1);
    EXPECT_EQ(FixFramer().next(start).status, Frame::Status::Invalid);
    EXPECT_EQ(FixFramer().next("8=FIX.4.4" + soh + "9=99999999999999999999999").status, Frame::Status::Invalid);
}

TEST(FixFramer, BodyLengthTooShortOrTooLong_IsInvalid) {
    // The trailer has to start exactly where BodyLength says the body ends
    const std::string tooShort = "8=FIX.4.4" + soh + "9=" + std::to_string(body.size() - 1) + soh + body + "10=123" + soh;
    EXPECT_EQ(FixFramer().next(tooShort).status, Frame::Status::Invalid);

    const std::string tooLong = "8=FIX.4.4" + soh + "9=" + std::to_string(body.size() + 1) + soh + body + "10=123" + soh + "8";
    EXPECT_EQ(FixFramer().next(tooLong).status, Frame::Status::Invalid);
}

TEST(FixFramer, MalformedTrailer_IsInvalid) {
    const std::string start = "8=FIX.4.4" + soh + "9=" + std::to_string(body.size()) + soh + body;
    EXPECT_EQ(FixFramer().next(start + "10=12x" + soh).status, Frame::Status::Invalid);
    EXPECT_EQ(FixFramer().next(start + "10=123\n").status, Frame::Status::Invalid);
}

// ── LineFramer ───────────────────────────────────────────────────────────────

TEST(LineFramer, Line_IsCompleteWithoutTheNewline) {
    Frame f = LineFramer().next("{\"a\":1}\n{\"b\"");
    ASSERT_EQ(f.status, Frame::Status::Complete);
    EXPECT_EQ(f.consumed, 8u);
    EXPECT_EQ(f.message, "{\"a\":1}");
}

TEST(LineFramer, NoNewline_IsIncomplete) {
    EXPECT_EQ(LineFramer().next("{\"a\":1}").status, Frame::Status::Incomplete);
    EXPECT_EQ(LineFramer().next("").status, Frame::Status::Incomplete);
}

TEST(LineFramer, CrLfLineEnding_CrIsRemoved) {
    Frame f = LineFramer().next("snapshot BTC\r\nnext");
    ASSERT_EQ(f.status, Frame::Status::Complete);
    EXPECT_EQ(f.consumed, 14u);
    EXPECT_EQ(f.message, "snapshot BTC");
    EXPECT_EQ(LineFramer().next("a\rb\n").message, "a\rb"); // only a '\r' right before the '\n' is a line ending
}

TEST(LineFramer, OverlongLine_IsInvalid) {
    const std::size_t max = LineFramer::kMaxMessageSize;
    EXPECT_EQ(LineFramer().next(std::string(max + 1, 'x') + "\n").status, Frame::Status::Invalid);
    EXPECT_EQ(LineFramer().next(std::string(max + 2, 'x')).status, Frame::Status::Invalid); // before any newline
    EXPECT_EQ(LineFramer().next(std::string(max, 'x') + "\n").status, Frame::Status::Complete);
    EXPECT_EQ(LineFramer().next(std::string(max, 'x') + "\r\n").status, Frame::Status::Complete);
    EXPECT_EQ(LineFramer().next(std::string(max, 'x') + "\r").status, Frame::Status::Incomplete); // "\n" may follow
}
