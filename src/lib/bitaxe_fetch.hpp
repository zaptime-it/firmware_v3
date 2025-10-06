#pragma once

#include <Arduino.h>
#include <HTTPClient.h>
#include <utils.hpp>

#include "lib/config.hpp"
#include "lib/shared.hpp"

class BitaxeFetch {
public:
    static BitaxeFetch& getInstance() {
        static BitaxeFetch instance;
        return instance;
    }

    void setup();
    uint64_t getHashRate() const;
    uint64_t getBestDiff() const;
    static void taskWrapper(void* pvParameters);
    TaskHandle_t getTaskHandle() const { return taskHandle; }

private:
    BitaxeFetch() = default;
    ~BitaxeFetch() = default;
    BitaxeFetch(const BitaxeFetch&) = delete;
    BitaxeFetch& operator=(const BitaxeFetch&) = delete;

    void task();
    
    TaskHandle_t taskHandle = nullptr;
    uint64_t hashrate = 0;
    uint64_t bestDiff = 0;
};