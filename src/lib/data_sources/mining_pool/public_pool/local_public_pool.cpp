#include "local_public_pool.hpp"
#include "lib/system/defaults.hpp"
#include "lib/system/shared.hpp"

std::string LocalPublicPool::getEndpoint() const {
  return preferences.getString("localPoolHost", DEFAULT_LOCAL_POOL_ENDPOINT)
      .c_str();
}

std::string LocalPublicPool::getApiUrl() const {
  return "http://" + getEndpoint() + "/api/client/" + poolUser;
}