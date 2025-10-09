#include "local_public_pool.hpp"
#include "lib/shared.hpp"
#include "lib/defaults.hpp"

std::string LocalPublicPool::getEndpoint() const {
    return preferences.getString("localPoolEndpoint", DEFAULT_LOCAL_POOL_ENDPOINT).c_str();
}

std::string LocalPublicPool::getApiUrl() const {
    return "http://" + getEndpoint() + "/api/client/" + poolUser;
} 