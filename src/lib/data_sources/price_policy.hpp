#pragma once

#include <string>
#include <vector>

namespace price_policy {

// Parse a comma-separated actCurrencies string ("USD,EUR,JPY") into the
// list of uppercase currency codes we should subscribe to on Kraken.
//
// Split as a pure std::string helper (rather than inline in the Kraken
// WS callback that uses Arduino String + WiFi) so the trim/empty-skip
// behaviour is unit-testable in the native env. Concrete invariants the
// test locks in:
//   * every comma-separated token is trimmed of surrounding whitespace,
//   * empty tokens are dropped (trailing comma, back-to-back commas,
//     entirely blank input),
//   * the original order is preserved so the first configured currency
//     stays the "primary" one in later handling,
//   * codes are not uppercased or validated here — Kraken is
//     case-insensitive and the device ships them as-is.
inline std::vector<std::string> parseCurrencyCsv(const std::string& csv)
{
    std::vector<std::string> out;
    std::string::size_type start = 0;
    while (start <= csv.size()) {
        std::string::size_type comma = csv.find(',', start);
        if (comma == std::string::npos) comma = csv.size();

        std::string::size_type l = start;
        std::string::size_type r = comma;
        while (l < r && std::isspace(static_cast<unsigned char>(csv[l]))) ++l;
        while (r > l && std::isspace(static_cast<unsigned char>(csv[r - 1]))) --r;
        if (r > l) out.emplace_back(csv.substr(l, r - l));

        start = comma + 1;
    }
    return out;
}

}  // namespace price_policy
