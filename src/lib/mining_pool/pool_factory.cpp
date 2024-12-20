#include "pool_factory.hpp"

const char* PoolFactory::MINING_POOL_NAME_OCEAN = "ocean";
const char* PoolFactory::MINING_POOL_NAME_NODERUNNERS = "noderunners";
const char* PoolFactory::MINING_POOL_NAME_BRAIINS = "braiins";
const char* PoolFactory::MINING_POOL_NAME_SATOSHI_RADIO = "satoshi_radio";
const char* PoolFactory::MINING_POOL_NAME_PUBLIC_POOL = "public_pool";
const char* PoolFactory::MINING_POOL_NAME_GOBRRR_POOL = "gobrrr_pool";
const char* PoolFactory::LOGOS_DIR = "/logos";

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

void PoolFactory::downloadPoolLogo(const std::string& poolName, const MiningPoolInterface* poolInterface)
{
    const int MAX_RETRIES = 5;
    const int RETRY_DELAY_MS = 1000;  // 1 second between retries

    if (!poolInterface || !poolInterface->hasLogo()) {
        Serial.println(F("No pool interface or logo"));
        return;
    }

    // Ensure logos directory exists
    if (!LittleFS.exists(LOGOS_DIR)) {
        LittleFS.mkdir(LOGOS_DIR);
    }

    String logoPath = String(LOGOS_DIR) + "/" + String(poolName.c_str()) + "_logo.bin";

    // Only download if the logo doesn't exist
    if (!LittleFS.exists(logoPath)) {
        // Clean up logos directory first
        File root = LittleFS.open(LOGOS_DIR, "r");
        if (root) {
            File file = root.openNextFile();
            while (file) {
                String path = file.path();
                file.close();
                LittleFS.remove(path);
                file = root.openNextFile();
            }
            root.close();
        }

        // Download new logo with retries
        std::string logoUrl = poolInterface->getLogoUrl();
        if (!logoUrl.empty()) {
            for (int attempt = 1; attempt <= MAX_RETRIES; attempt++) {
                Serial.printf("Downloading pool logo (attempt %d of %d)...\n", attempt, MAX_RETRIES);
                
                HTTPClient http;
                http.setUserAgent(USER_AGENT);
                http.begin(logoUrl.c_str());
                int httpCode = http.GET();
                
                if (httpCode == 200) {
                    File file = LittleFS.open(logoPath, "w");
                    if (file) {
                        http.writeToStream(&file);
                        file.close();
                        Serial.println(F("Logo downloaded successfully"));
                        http.end();
                        return;  // Success!
                    }
                }
                
                http.end();
                
                if (attempt < MAX_RETRIES) {
                    Serial.printf("Failed to download logo, HTTP code: %d. Retrying...\n", httpCode);
                    vTaskDelay(pdMS_TO_TICKS(RETRY_DELAY_MS));
                } else {
                    Serial.printf("Failed to download logo after %d attempts\n", MAX_RETRIES);
                }
            }
        }
    } else {
        Serial.println(F("Logo already exists"));
    }
}

LogoData PoolFactory::loadLogoFromFS(const std::string& poolName, const MiningPoolInterface* poolInterface)
{
    // Initialize with dimensions from the pool interface
    LogoData logo = {nullptr, 
                     0, 
                     0, 
                     0};
    
    String logoPath = String(LOGOS_DIR) + "/" + String(poolName.c_str()) + "_logo.bin";
    if (!LittleFS.exists(logoPath)) {
        return logo;
    }
    
    // Only set dimensions if file exists
    logo.width = static_cast<size_t>(poolInterface->getLogoWidth());
    logo.height = static_cast<size_t>(poolInterface->getLogoHeight());

    File file = LittleFS.open(logoPath, "r");
    if (!file) {
        return logo;
    }

    size_t size = file.size();
    uint8_t* buffer = new uint8_t[size];


    if (file.read(buffer, size) == size) {
        logo.data = buffer;
        logo.size = size;
    } else {
        delete[] buffer;
        logo.data = nullptr;
        logo.size = 0;
    }

    file.close();
    return logo;
}