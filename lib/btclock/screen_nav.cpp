#include "screen_nav.hpp"

namespace btclock {

int nextCurrencyIndex(bool newScreenIsCurrencySpecific, bool forward,
                      int activeCurrenciesSize) {
  if (!newScreenIsCurrencySpecific)
    return -1;
  if (activeCurrenciesSize <= 0)
    return -1;
  return forward ? 0 : activeCurrenciesSize - 1;
}

} // namespace btclock
