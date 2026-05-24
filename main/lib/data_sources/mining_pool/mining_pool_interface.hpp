#pragma once

#include "lib/system/shared.hpp"
#include "logo_data.hpp"
#include "pool_stats.hpp"
#include <ArduinoJson.h>
#include <HTTPClient.h>

class MiningPoolInterface {
public:
  virtual ~MiningPoolInterface() = default;
  virtual void setPoolUser(const std::string &user) = 0;
  virtual void prepareRequest(HTTPClient &http) const = 0;
  virtual std::string getApiUrl() const = 0;
  virtual PoolStats parseResponse(const JsonDocument &doc) const = 0;
  virtual bool hasLogo() const = 0;
  virtual LogoData getLogo() const;
  virtual std::string getDisplayLabel() const = 0;
  virtual bool supportsDailyEarnings() const = 0;
  virtual std::string getDailyEarningsLabel() const = 0;
  virtual std::string getLogoFilename() const { return ""; }
  virtual std::string getPoolName() const = 0;
  virtual int getLogoWidth() const { return 0; }
  virtual int getLogoHeight() const { return 0; }

  // Some ckpool-family pools expose a /api/v1/pool endpoint whose JSON
  // has the same hashrate1m shape as the per-user endpoint. Pools that
  // implement this can opt in by overriding both methods; the fetch
  // task will pick the URL based on the poolGlobalStats preference.
  virtual bool supportsGlobalStats() const { return false; }
  virtual std::string getGlobalStatsUrl() const { return ""; }
  std::string getLogoUrl() const {
    if (!hasLogo() || getLogoFilename().empty()) {
      return "";
    }
    std::string baseUrl =
        preferences.getString("poolLogosUrl", DEFAULT_MINING_POOL_LOGOS_URL)
            .c_str();
    return baseUrl + "/" + getLogoFilename().c_str();
  }

protected:
  std::string poolUser;
};