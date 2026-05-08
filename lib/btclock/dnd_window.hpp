#pragma once

#include <cstdint>

// Do-Not-Disturb time-window algebra.
//
// Extracted into lib/btclock/ so it can be linked into the native
// unit-test environment without pulling in the rest of LedHandler
// (which depends on Arduino, Preferences, NeoPixel and FreeRTOS).
//
// Contract:
//   * Range is half-open: the window includes startHour:startMinute
//     and excludes endHour:endMinute.
//   * If endTime < startTime the window wraps midnight, e.g.
//     22:30 -> 07:00 means "22:30 through 06:59".
//   * Start == End is treated as an empty window (never active),
//     not a 24-hour window, so the user cannot accidentally lock
//     the device into permanent DND by picking the same time twice.
//   * Hour/minute values must be in 0-23 / 0-59; out-of-range values
//     produce undefined (but bounded) behaviour, not a crash.

namespace btclock {

bool isTimeInDNDRange(uint8_t hour, uint8_t minute, uint8_t startHour,
                      uint8_t startMinute, uint8_t endHour, uint8_t endMinute);

} // namespace btclock
