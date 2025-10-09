#include "mining_pool_interface.hpp"
#include "pool_factory.hpp"

LogoData MiningPoolInterface::getLogo() const {
    if (!hasLogo()) {
        return LogoData{nullptr, 0, 0, 0};
    }
    
    // Check if logo exists 
    String logoPath = String(PoolFactory::getLogosDir()) + "/" + String(getPoolName().c_str()) + "_logo.bin";

    if (!LittleFS.exists(logoPath)) {
        return LogoData{nullptr, 0, 0, 0};
    }
    
    // Now load the logo (whether it was just downloaded or already existed)
    return PoolFactory::loadLogoFromFS(getPoolName(), this);
} 