
#pragma once

#include <optional>
#include <string>

struct PoolStats {
  std::string hashrate;
  std::optional<int64_t> dailyEarnings;
};
