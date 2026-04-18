#pragma once

// Pure helpers for screen / currency navigation.
//
// Extracted into lib/btclock/ so it can be linked into the native
// unit-test environment without pulling in Arduino, Preferences,
// FreeRTOS, or the full ScreenHandler stack.

namespace btclock {

// Decide which currency to select when navigating onto a new screen.
//
// Returns the target currency index in the active-currency list:
//   * 0                       — first currency (forward transition)
//   * activeCurrenciesSize-1  — last currency (backward transition)
//   * -1                      — keep current currency, no reset needed
//
// The reset is driven by the *target* screen, not the screen we're
// leaving. This matters for the "leave cs-screen at first currency,
// go back to non-cs, then forward again" flow: the forward press
// lands on a cs-screen and must restart at the first currency even
// though the previous press silently walked currency state.
int nextCurrencyIndex(bool newScreenIsCurrencySpecific,
                      bool forward,
                      int activeCurrenciesSize);

}  // namespace btclock
