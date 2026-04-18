#pragma once

#include "public_pool.hpp"

class LocalPublicPool : public PublicPool {
public:
    std::string getApiUrl() const override;
    std::string getDisplayLabel() const override { return "LOCAL/POOL"; }
private:
    std::string getEndpoint() const;
}; 