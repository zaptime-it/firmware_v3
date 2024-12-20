#include "pool_factory.hpp"


const char* PoolFactory::MINING_POOL_NAME_OCEAN = "ocean";
const char* PoolFactory::MINING_POOL_NAME_NODERUNNERS = "noderunners";
const char* PoolFactory::MINING_POOL_NAME_BRAIINS = "braiins";

std::unique_ptr<MiningPoolInterface> PoolFactory::createPool(const std::string& poolName) {
    static const std::unordered_map<std::string, std::function<std::unique_ptr<MiningPoolInterface>()>> poolFactories = {
        {MINING_POOL_NAME_OCEAN, []() { return std::make_unique<OceanPool>(); }},
        {MINING_POOL_NAME_NODERUNNERS, []() { return std::make_unique<NodeRunnersPool>(); }},
        {MINING_POOL_NAME_BRAIINS, []() { return std::make_unique<BraiinsPool>(); }}
    };
    
    auto it = poolFactories.find(poolName);
    if (it == poolFactories.end()) {
        return nullptr;
    }
    return it->second();
}