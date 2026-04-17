
#pragma once

#include "lib/data_sources/mining_pool/mining_pool_interface.hpp"
#include "lib/data_sources/mining_pool/noderunners/noderunners_pool.hpp"

#include <icons/icons.h>

class SatoshiRadioPool : public NoderunnersPool {
public:
    std::string getApiUrl() const override;
    bool hasLogo() const override { return false; }
    std::string getDisplayLabel() const override { return "SATOSHI/RADIO"; } // Fallback if needed
    // Without this override the pool inherits "noderunners" from the parent
    // class, which confuses pool-specific caches (e.g. the downloaded-logo
    // filename) and settings diffing.
    std::string getPoolName() const override { return "satoshiradio"; }
};