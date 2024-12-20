#include "pool_factory.hpp"

const char* PoolFactory::MINING_POOL_NAME_OCEAN = "ocean";
const char* PoolFactory::MINING_POOL_NAME_NODERUNNERS = "noderunners";
const char* PoolFactory::MINING_POOL_NAME_BRAIINS = "braiins";
const char* PoolFactory::MINING_POOL_NAME_SATOSHI_RADIO = "satoshi_radio";
const char* PoolFactory::MINING_POOL_NAME_PUBLIC_POOL = "public_pool";
const char* PoolFactory::MINING_POOL_NAME_GOBRRR_POOL = "gobrrr_pool";

std::unique_ptr<MiningPoolInterface> PoolFactory::createPool(const std::string& poolName) {
    static const std::unordered_map<std::string, std::function<std::unique_ptr<MiningPoolInterface>()>> poolFactories = {
        {MINING_POOL_NAME_OCEAN, []() { return std::make_unique<OceanPool>(); }},
        {MINING_POOL_NAME_NODERUNNERS, []() { return std::make_unique<NoderunnersPool>(); }},
        {MINING_POOL_NAME_BRAIINS, []() { return std::make_unique<BraiinsPool>(); }},
        {MINING_POOL_NAME_SATOSHI_RADIO, []() { return std::make_unique<SatoshiRadioPool>(); }},
        {MINING_POOL_NAME_PUBLIC_POOL, []() { return std::make_unique<PublicPool>(); }},
        {MINING_POOL_NAME_GOBRRR_POOL, []() { return std::make_unique<GoBrrrPool>(); }}
    };
    
    auto it = poolFactories.find(poolName);
    if (it == poolFactories.end()) {
        return nullptr;
    }
    return it->second();
}