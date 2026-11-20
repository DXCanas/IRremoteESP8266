// Copyright 2026 David C

#include "IRac.h"
#include "IRrecv.h"
#include "IRrecv_test.h"
#include "IRsend.h"
#include "IRsend_test.h"
#include "gtest/gtest.h"

TEST(TestUtils, Housekeeping) {
  ASSERT_EQ("EUFY", typeToString(decode_type_t::EUFY));
  ASSERT_EQ(decode_type_t::EUFY, strToDecodeType("EUFY"));
  ASSERT_FALSE(hasACState(decode_type_t::EUFY));
  ASSERT_FALSE(IRac::isProtocolSupported(decode_type_t::EUFY));
  ASSERT_EQ(kEufyBits, IRsend::defaultBits(decode_type_t::EUFY));
  ASSERT_EQ(kEufyDefaultRepeat, IRsend::minRepeats(decode_type_t::EUFY));
}

// Tests for encodeEufy().
TEST(TestEncodeEufy, Checksum) {
  IRsendTest irsend(kGpioUnused);
  EXPECT_EQ(0x68A0000008, irsend.encodeEufy(0xA0));  // AUTO
  EXPECT_EQ(0x68F0000058, irsend.encodeEufy(0xF0));  // Start/Pause
  // Clock 13:44 and 23:51, as captured from a real remote.
  EXPECT_EQ(0x68450D2CE6, irsend.encodeEufy(0x45, 13, 44));
  EXPECT_EQ(0x68451733F7, irsend.encodeEufy(0x45, 23, 51));
  // Schedule 09:30: the checksum wraps.
  EXPECT_EQ(0x6856091EE5, irsend.encodeEufy(0x56, 9, 30));
}

// Tests for sendEufy().
TEST(TestSendEufy, SendDataOnly) {
  IRsendTest irsend(kGpioUnused);
  irsend.begin();
  irsend.reset();
  irsend.sendEufy(0x68A0000008);  // AUTO
  EXPECT_EQ(
      "f38000d50"
      "m3000s3030"
      "m520s490m520s1490m520s1490m520s490m520s1490m520s490m520s490m520s490"
      "m520s1490m520s490m520s1490m520s490m520s490m520s490m520s490m520s490"
      "m520s490m520s490m520s490m520s490m520s490m520s490m520s490m520s490"
      "m520s490m520s490m520s490m520s490m520s490m520s490m520s490m520s490"
      "m520s490m520s490m520s490m520s490m520s1490m520s490m520s490m520s490"
      "m520s29900",
      irsend.outputStr());
}

TEST(TestSendEufy, SendWithRepeats) {
  IRsendTest irsend(kGpioUnused);
  irsend.begin();
  irsend.reset();
  irsend.sendEufy(0x68A200000A, kEufyBits, 2);  // Up, 3 frames in total.
  EXPECT_EQ(
      "f38000d50"
      "m3000s3030"
      "m520s490m520s1490m520s1490m520s490m520s1490m520s490m520s490m520s490"
      "m520s1490m520s490m520s1490m520s490m520s490m520s490m520s1490m520s490"
      "m520s490m520s490m520s490m520s490m520s490m520s490m520s490m520s490"
      "m520s490m520s490m520s490m520s490m520s490m520s490m520s490m520s490"
      "m520s490m520s490m520s490m520s490m520s1490m520s490m520s1490m520s490"
      "m520s29900"
      "m3000s3030"
      "m520s490m520s1490m520s1490m520s490m520s1490m520s490m520s490m520s490"
      "m520s1490m520s490m520s1490m520s490m520s490m520s490m520s1490m520s490"
      "m520s490m520s490m520s490m520s490m520s490m520s490m520s490m520s490"
      "m520s490m520s490m520s490m520s490m520s490m520s490m520s490m520s490"
      "m520s490m520s490m520s490m520s490m520s1490m520s490m520s1490m520s490"
      "m520s29900"
      "m3000s3030"
      "m520s490m520s1490m520s1490m520s490m520s1490m520s490m520s490m520s490"
      "m520s1490m520s490m520s1490m520s490m520s490m520s490m520s1490m520s490"
      "m520s490m520s490m520s490m520s490m520s490m520s490m520s490m520s490"
      "m520s490m520s490m520s490m520s490m520s490m520s490m520s490m520s490"
      "m520s490m520s490m520s490m520s490m520s1490m520s490m520s1490m520s490"
      "m520s29900",
      irsend.outputStr());
}

// Tests for decodeEufy().
TEST(TestDecodeEufy, RealDecode) {
  IRsendTest irsend(kGpioUnused);
  IRrecv irrecv(kGpioUnused);
  irsend.begin();

  // AUTO, as three frames (one press) in one capture. Coredy R3500 remote.
const uint16_t rawAuto[251] = {
      3000, 3030, 530, 480, 530, 1490, 530, 1490, 530, 480, 530, 1495, 495,
      510, 500, 490, 495, 510, 495, 1520, 500, 505, 530, 1490, 530, 480, 530,
      480, 530, 480, 530, 480, 525, 480, 530, 480, 530, 480, 530, 480, 530,
      480, 530, 480, 530, 480, 525, 480, 500, 510, 500, 490, 495, 510, 500,
      510, 500, 510, 500, 510, 530, 480, 530, 480, 530, 480, 530, 480, 530,
      480, 530, 480, 530, 480, 530, 1490, 530, 480, 530, 480, 530, 480, 530,
      29510, 3000, 3030, 530, 480, 530, 1490, 530, 1490, 525, 480, 525, 1470,
      520, 505, 500, 510, 500, 490, 495, 1520, 500, 510, 530, 1490, 530, 480,
      500, 510, 525, 485, 530, 480, 500, 510, 525, 485, 500, 510, 525, 505,
      480, 530, 500, 510, 480, 530, 475, 510, 500, 510, 495, 510, 475, 535,
      475, 530, 500, 510, 470, 535, 480, 530, 500, 510, 480, 530, 500, 510,
      500, 505, 480, 530, 480, 530, 505, 1490, 530, 505, 505, 505, 505, 505,
      505, 29530, 3000, 3035, 530, 505, 505, 1490, 530, 1490, 530, 505, 505,
      1490, 525, 480, 500, 510, 495, 515, 495, 1500, 500, 510, 530, 1490, 530,
      480, 530, 480, 530, 480, 530, 480, 525, 480, 530, 480, 530, 480, 530,
      480, 530, 480, 530, 480, 530, 480, 525, 465, 540, 485, 500, 490, 490,
      515, 470, 540, 470, 540, 500, 510, 525, 480, 525, 480, 530, 480, 525,
      480, 525, 480, 530, 480, 525, 480, 525, 1495, 500, 505, 525, 485, 525,
      480, 525};
  irsend.reset();
  irsend.sendRaw(rawAuto, 251, 38);
  irsend.makeDecodeResult();
  ASSERT_TRUE(irrecv.decode(&irsend.capture));
  EXPECT_EQ(decode_type_t::EUFY, irsend.capture.decode_type);
  EXPECT_EQ(kEufyBits, irsend.capture.bits);
  EXPECT_EQ(0x68A0000008, irsend.capture.value);
  EXPECT_EQ(0x68, irsend.capture.address);
  EXPECT_EQ(0xA0, irsend.capture.command);

  // Up (first frame of the press).
  const uint16_t rawUp[83] = {
      2995, 3025, 530, 480, 525, 1485, 525, 1485, 530, 480, 530, 1485, 500,
      505, 500, 505, 475, 510, 500, 1515, 500, 505, 530, 1485, 530, 480, 530,
      480, 530, 480, 525, 1485, 530, 480, 525, 480, 530, 480, 530, 480, 530,
      480, 530, 480, 500, 490, 515, 510, 475, 515, 495, 510, 495, 510, 500,
      505, 500, 505, 540, 470, 530, 480, 525, 480, 530, 480, 530, 480, 530,
      480, 530, 480, 530, 480, 530, 1485, 530, 480, 530, 1485, 500, 510, 500};
  irsend.reset();
  irsend.sendRaw(rawUp, 83, 38);
  irsend.makeDecodeResult();
  ASSERT_TRUE(irrecv.decode(&irsend.capture));
  EXPECT_EQ(decode_type_t::EUFY, irsend.capture.decode_type);
  EXPECT_EQ(0x68A200000A, irsend.capture.value);
  EXPECT_EQ(0xA2, irsend.capture.command);

  // Clock set to 23:51 (first frame of the press).
  const uint16_t rawClock[83] = {
      2995, 3025, 530, 480, 530, 1485, 530, 1490, 525, 480, 530, 1485, 500,
      505, 500, 490, 495, 515, 495, 510, 495, 1515, 530, 480, 530, 480, 530,
      480, 530, 1490, 530, 480, 530, 1490, 530, 480, 530, 480, 530, 480, 530,
      1485, 500, 490, 495, 1515, 500, 1515, 530, 1485, 530, 480, 525, 480,
      530, 1485, 530, 1485, 530, 480, 530, 480, 530, 1465, 495, 1520, 500,
      1515, 530, 1490, 530, 1490, 530, 1490, 525, 480, 530, 1485, 530, 1485,
      500, 1495, 495};
  irsend.reset();
  irsend.sendRaw(rawClock, 83, 38);
  irsend.makeDecodeResult();
  ASSERT_TRUE(irrecv.decode(&irsend.capture));
  EXPECT_EQ(decode_type_t::EUFY, irsend.capture.decode_type);
  EXPECT_EQ(0x68451733F7, irsend.capture.value);
  EXPECT_EQ(0x45, irsend.capture.command);
}

// The remote's MAX button resends the previous message's hour, minute and
// checksum bytes, so its checksum is wrong. It must still decode.
TEST(TestDecodeEufy, RealMaxWithStaleChecksum) {
  IRsendTest irsend(kGpioUnused);
  IRrecv irrecv(kGpioUnused);
  irsend.begin();

  // MAX pressed after setting the clock to 23:51.
  const uint16_t rawMax[83] = {
      3000, 3025, 530, 480, 530, 1490, 530, 1485, 530, 480, 530, 1470, 520,
      505, 500, 510, 475, 510, 495, 510, 495, 510, 500, 510, 500, 1510, 530,
      1485, 530, 1485, 530, 1490, 530, 480, 530, 480, 530, 480, 530, 460, 520,
      1495, 495, 510, 495, 1515, 530, 1485, 530, 1485, 530, 480, 530, 480,
      525, 1490, 530, 1485, 530, 480, 525, 480, 500, 1490, 495, 1515, 530,
      1485, 530, 1485, 530, 1490, 530, 1490, 530, 480, 530, 1485, 500, 1495,
      495, 1515, 500};
  irsend.reset();
  irsend.sendRaw(rawMax, 83, 38);
  irsend.makeDecodeResult();
  ASSERT_TRUE(irrecv.decode(&irsend.capture));
  EXPECT_EQ(decode_type_t::EUFY, irsend.capture.decode_type);
  EXPECT_EQ(0x681E1733F7, irsend.capture.value);
  EXPECT_EQ(0x1E, irsend.capture.command);

  // MAX pressed after Spot (0x681200007A).
  const uint16_t rawMaxAfterSpot[83] = {
      3000, 3030, 520, 480, 530, 1490, 530, 1490, 530, 480, 530, 1490, 500,
      505, 500, 510, 470, 510, 495, 510, 500, 510, 500, 505, 530, 1490, 530,
      1485, 530, 1490, 530, 1490, 530, 480, 530, 480, 530, 480, 530, 480, 500,
      505, 500, 510, 475, 510, 495, 510, 495, 510, 500, 505, 530, 480, 530,
      480, 530, 480, 530, 480, 530, 480, 530, 480, 530, 480, 530, 480, 530,
      1490, 530, 1485, 530, 1485, 475, 1520, 495, 510, 500, 1515, 530, 480,
      530};
  irsend.reset();
  irsend.sendRaw(rawMaxAfterSpot, 83, 38);
  irsend.makeDecodeResult();
  ASSERT_TRUE(irrecv.decode(&irsend.capture));
  EXPECT_EQ(decode_type_t::EUFY, irsend.capture.decode_type);
  EXPECT_EQ(0x681E00007A, irsend.capture.value);
}

// A press from further away / off-axis: the receiver shortens the marks and
// lengthens the spaces (e.g. 445 & 565 instead of 520 & 490). It must decode.
TEST(TestDecodeEufy, RealWeakSignal) {
  IRsendTest irsend(kGpioUnused);
  IRrecv irrecv(kGpioUnused);
  irsend.begin();

  // MAX after Spot.
  const uint16_t rawWeakMax[83] = {
      2970, 3065, 445, 565, 440, 1555, 490, 1530, 470, 540, 490, 1525,
      470, 540, 470, 540, 465, 550, 465, 540, 470, 520, 490, 540, 470,
      1530, 470, 1555, 465, 1550, 470, 1550, 465, 545, 465, 540, 470, 540,
      470, 565, 445, 540, 470, 540, 490, 515, 465, 520, 490, 540, 440,
      545, 470, 540, 465, 540, 470, 565, 445, 565, 445, 565, 445, 565,
      470, 515, 470, 565, 470, 1550, 440, 1575, 445, 1550, 470, 1550, 465,
      520, 490, 1550, 440, 570, 440};
  irsend.reset();
  irsend.sendRaw(rawWeakMax, 83, 38);
  irsend.makeDecodeResult();
  ASSERT_TRUE(irrecv.decode(&irsend.capture));
  EXPECT_EQ(decode_type_t::EUFY, irsend.capture.decode_type);
  EXPECT_EQ(0x681E00007A, irsend.capture.value);
}

TEST(TestDecodeEufy, SyntheticSelfDecode) {
  IRsendTest irsend(kGpioUnused);
  IRrecv irrecv(kGpioUnused);
  irsend.begin();

  irsend.reset();
  irsend.sendEufy(irsend.encodeEufy(0x56, 9, 30), kEufyBits, 2);
  irsend.makeDecodeResult();
  ASSERT_TRUE(irrecv.decode(&irsend.capture));
  EXPECT_EQ(decode_type_t::EUFY, irsend.capture.decode_type);
  EXPECT_EQ(kEufyBits, irsend.capture.bits);
  EXPECT_EQ(0x6856091EE5, irsend.capture.value);
  EXPECT_EQ(0x56, irsend.capture.command);
}

TEST(TestDecodeEufy, StrictNeedsTheFixedFirstByte) {
  IRsendTest irsend(kGpioUnused);
  IRrecv irrecv(kGpioUnused);
  irsend.begin();

  irsend.reset();
  irsend.sendEufy(0x69A0000009);  // First byte 0x69, not 0x68.
  irsend.makeDecodeResult();
  EXPECT_FALSE(irrecv.decodeEufy(&irsend.capture, kStartOffset, kEufyBits,
                                 true));
  ASSERT_TRUE(irrecv.decodeEufy(&irsend.capture, kStartOffset, kEufyBits,
                                false));
  EXPECT_EQ(0x69A0000009, irsend.capture.value);
  // Too few bits to hold a command.
  EXPECT_FALSE(irrecv.decodeEufy(&irsend.capture, kStartOffset, 8, false));
}

TEST(TestDecodeEufy, OtherProtocolsAreNotEufy) {
  IRsendTest irsend(kGpioUnused);
  IRrecv irrecv(kGpioUnused);
  irsend.begin();

  irsend.reset();
  irsend.sendNEC(0xFFA857);
  irsend.makeDecodeResult();
  EXPECT_FALSE(irrecv.decodeEufy(&irsend.capture));
  ASSERT_TRUE(irrecv.decode(&irsend.capture));
  EXPECT_EQ(decode_type_t::NEC, irsend.capture.decode_type);

  irsend.reset();
  irsend.sendEufy(0x68A0000008);
  irsend.makeDecodeResult();
  EXPECT_FALSE(irrecv.decodeNEC(&irsend.capture));
}
