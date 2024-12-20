
#pragma once

#include "lib/mining_pool/mining_pool_interface.hpp"
#include "lib/mining_pool/public_pool/public_pool.hpp"

#include <icons/icons.h>

class GoBrrrPool : public PublicPool {
public:
    std::string getApiUrl() const override;
    bool hasLogo() const override { return true; }
    std::string getDisplayLabel() const override { return "GOBRRR/POOL"; }
    LogoData getLogo() const override;
};