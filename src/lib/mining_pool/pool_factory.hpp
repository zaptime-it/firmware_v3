#pragma once
#include "mining_pool_interface.hpp"
#include <memory>
#include <string>
#include "noderunners/noderunners_pool.hpp"
#include "braiins/brains_pool.hpp"
#include "ocean/ocean_pool.hpp"

class PoolFactory {
    public:
        static std::unique_ptr<MiningPoolInterface> createPool(const std::string& poolName);
        static std::vector<std::string> getAvailablePools() {
        return {
            MINING_POOL_NAME_OCEAN,
            MINING_POOL_NAME_NODERUNNERS,
            MINING_POOL_NAME_BRAIINS
        };
    }
    
    static std::string getAvailablePoolsAsString() {
        const auto pools = getAvailablePools();
        std::string result;
        for (size_t i = 0; i < pools.size(); ++i) {
            result += pools[i];
            if (i < pools.size() - 1) {
                result += ", ";
            }
        }
        return result;
    }
    private:
        static const char* MINING_POOL_NAME_OCEAN;
        static const char* MINING_POOL_NAME_NODERUNNERS;
        static const char* MINING_POOL_NAME_BRAIINS;
};