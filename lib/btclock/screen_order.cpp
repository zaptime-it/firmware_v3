#include "screen_order.hpp"

#include <cctype>
#include <sstream>
#include <unordered_set>

namespace btclock {

namespace {

std::string trim(const std::string &s) {
  auto begin = s.begin();
  while (begin != s.end() && std::isspace(static_cast<unsigned char>(*begin))) {
    ++begin;
  }
  auto end = s.end();
  while (end != begin && std::isspace(static_cast<unsigned char>(*(end - 1)))) {
    --end;
  }
  return std::string(begin, end);
}

// std::stoi throws on malformed input and also happily accepts "42abc"
// as 42. We reject either case explicitly so a typo in NVS doesn't
// quietly become a valid-looking screen ID.
bool parseIntStrict(const std::string &token, int &out) {
  if (token.empty())
    return false;
  size_t i = 0;
  if (token[0] == '-' || token[0] == '+')
    i = 1;
  if (i >= token.size())
    return false;
  for (; i < token.size(); ++i) {
    if (!std::isdigit(static_cast<unsigned char>(token[i])))
      return false;
  }
  try {
    out = std::stoi(token);
    return true;
  } catch (...) {
    return false;
  }
}

} // namespace

std::vector<int> parseScreenOrder(const std::string &csv) {
  std::vector<int> out;
  std::stringstream ss(csv);
  std::string token;
  while (std::getline(ss, token, ',')) {
    std::string trimmed = trim(token);
    if (trimmed.empty())
      continue;
    int id = 0;
    if (parseIntStrict(trimmed, id)) {
      out.push_back(id);
    }
  }
  return out;
}

std::string serializeScreenOrder(const std::vector<int> &order) {
  std::string out;
  for (size_t i = 0; i < order.size(); ++i) {
    if (i)
      out.push_back(',');
    out += std::to_string(order[i]);
  }
  return out;
}

std::vector<int> mergeScreenOrder(const std::vector<int> &stored,
                                  const std::vector<int> &catalog) {
  std::unordered_set<int> catalogSet(catalog.begin(), catalog.end());
  std::unordered_set<int> seen;
  std::vector<int> out;
  out.reserve(catalog.size());

  for (int id : stored) {
    if (catalogSet.find(id) == catalogSet.end())
      continue;
    if (!seen.insert(id).second)
      continue;
    out.push_back(id);
  }
  for (int id : catalog) {
    if (seen.insert(id).second) {
      out.push_back(id);
    }
  }
  return out;
}

} // namespace btclock
