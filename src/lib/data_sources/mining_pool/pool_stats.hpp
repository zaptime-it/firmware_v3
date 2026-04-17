
#pragma once

#include <string>
#include <optional>

struct PoolStats {
    std::string hashrate;
    std::optional<int64_t> dailyEarnings;
};
