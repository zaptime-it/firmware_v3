#pragma once

// Pure helpers for the user-configurable screen rotation order.
//
// Extracted into lib/btclock/ so parsing and catalog-merge logic can be
// linked into the native unit-test environment without pulling in
// Arduino, Preferences, or the full ScreenHandler stack.

#include <string>
#include <vector>

namespace btclock {

// Parse a comma-separated list of decimal screen IDs.
//
// Tolerant by design: trims whitespace around each token, skips empty
// tokens (including leading/trailing commas), and silently drops tokens
// that don't parse as a signed decimal integer. Does not deduplicate
// and does not validate against any catalog — that's mergeScreenOrder's
// job. Returns an empty vector for an empty / all-malformed input, at
// which point callers should fall back to a default.
std::vector<int> parseScreenOrder(const std::string &csv);

// Serialise the inverse of parseScreenOrder: comma-joined decimal IDs,
// no trailing comma, no whitespace. Empty vector → empty string.
std::string serializeScreenOrder(const std::vector<int> &order);

// Merge a user-stored order with the catalog of screens the current
// firmware supports (respecting feature flags).
//
// Rules, in order of precedence:
//   * An ID in `stored` that is also in `catalog` keeps its stored
//     position. First occurrence wins; duplicates are discarded.
//   * An ID in `stored` that is NOT in `catalog` (unknown, removed, or
//     feature-disabled in this firmware) is silently dropped.
//   * Any ID in `catalog` not mentioned in `stored` is appended at the
//     end, in catalog order. This is the graceful-upgrade path: a new
//     screen added by a firmware update appears last in rotation until
//     the user drags it elsewhere.
std::vector<int> mergeScreenOrder(const std::vector<int> &stored,
                                  const std::vector<int> &catalog);

} // namespace btclock
