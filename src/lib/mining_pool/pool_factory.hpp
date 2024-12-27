#pragma once
#include "mining_pool_interface.hpp"
#include <memory>
#include <string>
#include "lib/shared.hpp"
#include "lib/config.hpp"

#include "noderunners/noderunners_pool.hpp"
#include "braiins/brains_pool.hpp"
#include "ocean/ocean_pool.hpp"
#include "satoshi_radio/satoshi_radio_pool.hpp"
#include "public_pool/public_pool.hpp"
#include "gobrrr_pool/gobrrr_pool.hpp"
#include <LittleFS.h>
#include <HTTPClient.h>


class PoolFactory {
    public:
        static const char* getLogosDir() { return LOGOS_DIR; }
        static std::unique_ptr<MiningPoolInterface> createPool(const std::string& poolName);
        static std::vector<std::string> getAvailablePools() {
        return {
            MINING_POOL_NAME_OCEAN,
            MINING_POOL_NAME_NODERUNNERS,
            MINING_POOL_NAME_SATOSHI_RADIO,
            MINING_POOL_NAME_BRAIINS,
            MINING_POOL_NAME_PUBLIC_POOL,
            MINING_POOL_NAME_GOBRRR_POOL
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

    static void downloadPoolLogo(const std::string& poolName, const MiningPoolInterface* poolInterface);
    static LogoData loadLogoFromFS(const std::string& poolName, const MiningPoolInterface* poolInterface);

    private:
        static const char* MINING_POOL_NAME_OCEAN;
        static const char* MINING_POOL_NAME_NODERUNNERS;
        static const char* MINING_POOL_NAME_BRAIINS;
        static const char* MINING_POOL_NAME_SATOSHI_RADIO;
        static const char* MINING_POOL_NAME_PUBLIC_POOL;
        static const char* MINING_POOL_NAME_GOBRRR_POOL;
        static const char* LOGOS_DIR;
};