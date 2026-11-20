// Copyright 2026 David C

/// @file
/// @brief Support for the Eufy robot vacuum IR protocol.

// Supports:
//   Brand: Eufy,  Model: RoboVac 11 (T2102) Robot Vacuum
//   Brand: Coredy,  Model: R3500 Robot Vacuum
//   Brand: Coredy,  Model: Y3B4A2 Remote

// A 40-bit message, most significant bit first:
//   byte 0: 0x68 (fixed)
//   byte 1: command
//   byte 2: hour   (0-23) for the clock and schedule commands, else 0
//   byte 3: minute (0-59) for the clock and schedule commands, else 0
//   byte 4: checksum, the sum of bytes 0-3 (mod 256)
// The remote repeats the message (usually 3 frames, ~30ms apart) while a
// button is held. A robot acts on a single frame. A held arrow button keeps
// the robot moving while frames keep arriving.
//
// Command   Button (Coredy R3500 remote)   Effect (tested on a Coredy R3500)
// =======   ============================   =================================
// 0x0F      Home / Re-charging             Return to the dock.
// 0x12      Spot                           Start a spot clean.
// 0x1E      MAX                            Toggle max suction while cleaning.
// 0x23      Edge                           Start an edge clean.
// 0x34      Small-space / Single room      Start that clean.
// 0x45      Clock (hh, mm)                 Set the clock. One beep.
// 0x56      Schedule (hh, mm)              Set a daily clean. One beep.
// 0x64      Cancel schedule                Two beeps.
// 0xA0      AUTO                           Start an auto clean.
// 0xA2      Up                             Drive forward.
// 0xB3      Down                           Reverse (a short distance).
// 0xC6      Left                           Rotate left in place.
// 0xD5      Right                          Rotate right in place.
// 0xF0      Start/Pause                    Pause a clean.
//
// Checksum: the decoder doesn't verify it. The Coredy remote's MAX button
// sends the hour, minute and checksum bytes left over from the previous
// button, so its checksum is usually wrong (e.g. 0x681E00007A after Spot),
// and the robot accepts any checksum. Rejecting such messages would make
// every real MAX press undecodable. The fixed first byte and the length are
// still checked. `encodeEufy()` always computes a correct checksum.
//
// The Eufy RoboVac 11's codes are the same as the Coredy's for every button
// they share (Flipper-IRDB: Vacuum_Cleaners/Eufy/Robovac_11.ir), except its
// "Max" button, which uses command 0x13.

#include "IRrecv.h"
#include "IRsend.h"
#include "IRutils.h"

// Constants
// Measured from 94 frames of a Coredy R3500 remote (Y3B4A2).
const uint16_t kEufyHdrMark = 3000;
const uint16_t kEufyHdrSpace = 3030;
const uint16_t kEufyBitMark = 520;
const uint16_t kEufyOneSpace = 1490;
const uint16_t kEufyZeroSpace = 490;
const uint32_t kEufyGap = 29900;  // Between repeated frames.
const uint16_t kEufyFreq = 38000;  // Hz. (Not measured.)
const uint8_t kEufyHeader = 0x68;

#if SEND_EUFY
/// Send a Eufy robot vacuum formatted message.
/// Status: STABLE / Confirmed working on a Coredy R3500.
/// @param[in] data The message to be sent. e.g. 0x68A0000008 (AUTO)
/// @param[in] nbits The number of bits of message to be sent.
/// @param[in] repeat The number of times the command is to be repeated.
/// @see encodeEufy()
void IRsend::sendEufy(const uint64_t data, const uint16_t nbits,
                      const uint16_t repeat) {
  sendGeneric(kEufyHdrMark, kEufyHdrSpace,
              kEufyBitMark, kEufyOneSpace,
              kEufyBitMark, kEufyZeroSpace,
              kEufyBitMark, kEufyGap,
              data, nbits, kEufyFreq, true, repeat, kDutyDefault);
}

/// Construct a raw Eufy robot vacuum message, with its checksum.
/// Status: STABLE / Confirmed working on a Coredy R3500.
/// @param[in] command The command byte. e.g. 0xA0 (AUTO)
/// @param[in] hour The hour (0-23), for the clock and schedule commands.
/// @param[in] minute The minute (0-59), for the clock and schedule commands.
/// @return A raw 40-bit message suitable for `sendEufy()`.
uint64_t IRsend::encodeEufy(const uint8_t command, const uint8_t hour,
                            const uint8_t minute) {
  const uint8_t checksum = kEufyHeader + command + hour + minute;
  uint64_t result = kEufyHeader;
  result = (result << 8) | command;
  result = (result << 8) | hour;
  result = (result << 8) | minute;
  return (result << 8) | checksum;
}
#endif  // SEND_EUFY

#if DECODE_EUFY
/// Decode the supplied Eufy robot vacuum message.
/// Status: STABLE / Confirmed working on a Coredy R3500.
/// @param[in,out] results Ptr to the data to decode & where to store the result
/// @param[in] offset The starting index to use when attempting to decode the
///   raw data. Typically/Defaults to kStartOffset.
/// @param[in] nbits The number of data bits to expect.
/// @param[in] strict Flag indicating if we should perform strict matching.
/// @return True if it can decode it, false if it can't.
/// @note The checksum is not verified. See the comment at the top of the file.
bool IRrecv::decodeEufy(decode_results *results, uint16_t offset,
                        const uint16_t nbits, const bool strict) {
  if (strict && nbits != kEufyBits)
    return false;  // We expect Eufy to be a certain sized message.
  if (nbits < 16 || nbits > 64)
    return false;  // Can't hold the header and a command, or too big.

  uint64_t data = 0;

  // Match Header + Data + Footer
  // No mark excess: a weak signal (distance, angle) comes in with shorter
  // marks and longer spaces (e.g. 445 & 565 for 520 & 490). With the default
  // excess, those zero spaces fall outside the tolerance.
  if (!matchGeneric(results->rawbuf + offset, &data,
                    results->rawlen - offset, nbits,
                    kEufyHdrMark, kEufyHdrSpace,
                    kEufyBitMark, kEufyOneSpace,
                    kEufyBitMark, kEufyZeroSpace,
                    kEufyBitMark, kEufyGap, true,
                    _tolerance, 0)) return false;

  // Compliance
  if (strict && (data >> (nbits - 8)) != kEufyHeader) return false;

  // Success
  results->bits = nbits;
  results->value = data;
  results->decode_type = decode_type_t::EUFY;
  results->address = data >> (nbits - 8);
  results->command = (data >> (nbits - 16)) & 0xFF;
  return true;
}
#endif  // DECODE_EUFY
