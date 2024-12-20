#pragma once

#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "pool_stats.hpp"
#include "logo_data.hpp"

class MiningPoolInterface {
public:
    virtual ~MiningPoolInterface() = default;
    virtual void setPoolUser(const std::string& user) = 0;
    virtual void prepareRequest(HTTPClient& http) const = 0;
    virtual std::string getApiUrl() const = 0;
    virtual PoolStats parseResponse(const JsonDocument& doc) const = 0;
    virtual bool hasLogo() const = 0;
    virtual LogoData getLogo() const = 0;
    virtual std::string getDisplayLabel() const = 0;
    virtual bool supportsDailyEarnings() const = 0;
    virtual std::string getDailyEarningsLabel() const = 0;

protected:
    std::string poolUser;
};