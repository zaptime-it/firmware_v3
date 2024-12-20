#pragma once

#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "pool_stats.hpp"
#include "logo_data.hpp"
#include "lib/shared.hpp"

class MiningPoolInterface {
public:
    virtual ~MiningPoolInterface() = default;
    virtual void setPoolUser(const std::string& user) = 0;
    virtual void prepareRequest(HTTPClient& http) const = 0;
    virtual std::string getApiUrl() const = 0;
    virtual PoolStats parseResponse(const JsonDocument& doc) const = 0;
    virtual bool hasLogo() const = 0;
    virtual LogoData getLogo() const;
    virtual std::string getDisplayLabel() const = 0;
    virtual bool supportsDailyEarnings() const = 0;
    virtual std::string getDailyEarningsLabel() const = 0;
    virtual std::string getLogoFilename() const { return ""; }
    virtual std::string getPoolName() const = 0;
    virtual int getLogoWidth() const { return 0; }
    virtual int getLogoHeight() const { return 0; }
    std::string getLogoUrl() const {
        if (!hasLogo() || getLogoFilename().empty()) {
            return "";
        }
        std::string baseUrl = preferences.getString("poolLogosUrl", DEFAULT_MINING_POOL_LOGOS_URL).c_str();
        return baseUrl + "/" + getLogoFilename().c_str();
    }

protected:
    std::string poolUser;
};