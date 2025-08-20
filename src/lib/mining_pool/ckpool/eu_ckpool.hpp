#pragma once

#include "ckpool.hpp"

class EUCKPool : public CKPool {
public:
    std::string getDisplayLabel() const override { return "CK/POOL"; }
    std::string getPoolName() const override {
        return "eu_ckpool";
    }

protected:
    std::string getBaseUrl() const override {
        return "https://eusolo.ckpool.org";
    }
}; 