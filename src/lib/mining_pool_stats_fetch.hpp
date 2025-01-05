#pragma once

#include <Arduino.h>
#include <HTTPClient.h>
#include <utils.hpp>
#include <memory>

#include "lib/config.hpp"
#include "lib/shared.hpp"
#include "lib/mining_pool/mining_pool_interface.hpp"
#include "mining_pool/pool_factory.hpp"

class MiningPoolStatsFetch {
public:
    static MiningPoolStatsFetch& getInstance() {
        static MiningPoolStatsFetch instance;
        return instance;
    }

    void setup();
    std::string getHashRate() const;
    int getDailyEarnings() const;
    TaskHandle_t getTaskHandle() const { return taskHandle; }
    static void taskWrapper(void* pvParameters);
    static void downloadLogoTaskWrapper(void* pvParameters);
    
    // Pool interface methods
    MiningPoolInterface* getPool();
    const MiningPoolInterface* getPool() const;
    LogoData getLogo() const;

private:
    MiningPoolStatsFetch() = default;
    ~MiningPoolStatsFetch() = default;
    MiningPoolStatsFetch(const MiningPoolStatsFetch&) = delete;
    MiningPoolStatsFetch& operator=(const MiningPoolStatsFetch&) = delete;

    void task();
    void downloadLogoTask();
    
    TaskHandle_t taskHandle = nullptr;
    std::string hashrate;
    int dailyEarnings = 0;
    std::unique_ptr<MiningPoolInterface> currentPool;
};