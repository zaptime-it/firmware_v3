
#pragma once

#include "lib/mining_pool/mining_pool_interface.hpp"
#include "lib/mining_pool/noderunners/noderunners_pool.hpp"

#include <icons/icons.h>

class PublicPool : public NoderunnersPool {
public:
    std::string getApiUrl() const override;
    bool hasLogo() const override { return false; }
    std::string getDisplayLabel() const override { return "PUBLIC/POOL"; }
    PoolStats parseResponse(const JsonDocument& doc) const override;
};