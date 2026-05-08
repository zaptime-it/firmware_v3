#include "dnd_window.hpp"

namespace btclock {

bool isTimeInDNDRange(uint8_t hour, uint8_t minute, uint8_t startHour,
                      uint8_t startMinute, uint8_t endHour, uint8_t endMinute) {
  // Convert everything to minutes-past-midnight so we only have to
  // compare a single integer and wrap-around stays arithmetic, not
  // special-case.
  const uint16_t currentTime =
      static_cast<uint16_t>(hour) * 60u + static_cast<uint16_t>(minute);
  const uint16_t startTime = static_cast<uint16_t>(startHour) * 60u +
                             static_cast<uint16_t>(startMinute);
  const uint16_t endTime =
      static_cast<uint16_t>(endHour) * 60u + static_cast<uint16_t>(endMinute);

  // Previously behaved as "always active" because `>= start` AND
  // `< start` is vacuously false on the wrap branch but `>= start`
  // on the same-day branch is true for exactly one value. Treat
  // degenerate ranges as "never active" so DND can't be switched on
  // permanently by the user picking the same start/end.
  if (startTime == endTime) {
    return false;
  }

  if (startTime < endTime) {
    return currentTime >= startTime && currentTime < endTime;
  }
  return currentTime >= startTime || currentTime < endTime;
}

} // namespace btclock
