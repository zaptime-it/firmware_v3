#include "screen_handler.hpp"

TaskHandle_t taskScreenRotateTaskHandle;
TaskHandle_t workerTaskHandle;
QueueHandle_t workQueue = NULL;

// Initialize static members
uint ScreenHandler::currentScreen = SCREEN_BLOCK_HEIGHT;
uint ScreenHandler::currentCurrency = CURRENCY_USD;

std::array<std::string, NUM_SCREENS> taskEpdContent = {};

// Convert existing functions to static member functions
void ScreenHandler::setCurrentScreen(uint newScreen) {
    if (newScreen != SCREEN_CUSTOM) {
        preferences.putUInt("currentScreen", newScreen);
    }
    currentScreen = newScreen;

    switch (currentScreen) {
        case SCREEN_TIME: {
            WorkItem timeUpdate = {TASK_TIME_UPDATE, 0};
            xQueueSend(workQueue, &timeUpdate, portMAX_DELAY);
            break;
        }
        case SCREEN_HALVING_COUNTDOWN:
        case SCREEN_BLOCK_HEIGHT: {
            WorkItem blockUpdate = {TASK_BLOCK_UPDATE, 0};
            xQueueSend(workQueue, &blockUpdate, portMAX_DELAY);
            break;
        }
        case SCREEN_MARKET_CAP:
        case SCREEN_SATS_PER_CURRENCY:
        case SCREEN_BTC_TICKER: {
            WorkItem priceUpdate = {TASK_PRICE_UPDATE, 0};
            xQueueSend(workQueue, &priceUpdate, portMAX_DELAY);
            break;
        }
        case SCREEN_BLOCK_FEE_RATE: {
            WorkItem blockUpdate = {TASK_FEE_UPDATE, 0};
            xQueueSend(workQueue, &blockUpdate, portMAX_DELAY);
            break;
        }
        case SCREEN_BITAXE_BESTDIFF:
        case SCREEN_BITAXE_HASHRATE: {
            if (preferences.getBool("bitaxeEnabled", DEFAULT_BITAXE_ENABLED)) {
                WorkItem bitaxeUpdate = {TASK_BITAXE_UPDATE, 0};
                xQueueSend(workQueue, &bitaxeUpdate, portMAX_DELAY);
            } else {
                setCurrentScreen(SCREEN_BLOCK_HEIGHT);
                return;
            }
            break;
        }
        case SCREEN_MINING_POOL_STATS_HASHRATE:
        case SCREEN_MINING_POOL_STATS_EARNINGS: {
            if (preferences.getBool("miningPoolStats", DEFAULT_MINING_POOL_STATS_ENABLED)) {
                WorkItem miningPoolStatsUpdate = {TASK_MINING_POOL_STATS_UPDATE, 0};
                xQueueSend(workQueue, &miningPoolStatsUpdate, portMAX_DELAY);
            } else {
                setCurrentScreen(SCREEN_BLOCK_HEIGHT);
                return;
            }
            break;
        }
    }

    if (eventSourceTaskHandle != NULL) xTaskNotifyGive(eventSourceTaskHandle);
}

void ScreenHandler::setCurrentCurrency(char currency) {
    currentCurrency = currency;
    preferences.putUChar("lastCurrency", currency);
}

bool ScreenHandler::isCurrencySpecific(uint screen) {
    switch (screen) {
        case SCREEN_BTC_TICKER:
        case SCREEN_SATS_PER_CURRENCY:
        case SCREEN_MARKET_CAP:
            return true;
        default:
            return false;
    }
}

bool ScreenHandler::handleCurrencyRotation(bool forward) {
    if ((getDataSource() == BTCLOCK_SOURCE || getDataSource() == CUSTOM_SOURCE) && isCurrencySpecific(getCurrentScreen())) {
        std::vector<std::string> ac = getActiveCurrencies();
        if (ac.empty()) return false;

        std::string curCode = getCurrencyCode(getCurrentCurrency());
        auto it = std::find(ac.begin(), ac.end(), curCode);
        
        if (it == ac.end()) {
            // Current currency not found in active currencies - initialize based on direction
            setCurrentCurrency(getCurrencyChar(forward ? ac.front() : ac.back()));
            setCurrentScreen(getCurrentScreen());
            return true;
        } else if (forward && curCode != ac.back()) {
            // Moving forward and not at last currency
            setCurrentCurrency(getCurrencyChar(ac.at(std::distance(ac.begin(), it) + 1)));
            setCurrentScreen(getCurrentScreen());
            return true;
        } else if (!forward && curCode != ac.front()) {
            // Moving backward and not at first currency
            setCurrentCurrency(getCurrencyChar(ac.at(std::distance(ac.begin(), it) - 1)));
            setCurrentScreen(getCurrentScreen());
            return true;
        }
        // If we're at the last/first currency of current screen, let nextScreen/previousScreen handle it
        return false;
    }
    return false;
}

int ScreenHandler::findNextVisibleScreen(int currentScreen, bool forward) {
    std::vector<ScreenMapping> screenMappings = getScreenNameMap();
    int newScreen;
    
    if (forward) {
        newScreen = (currentScreen < screenMappings.size() - 1) ? 
            screenMappings[currentScreen + 1].value : screenMappings.front().value;
    } else {
        newScreen = (currentScreen > 0) ? 
            screenMappings[currentScreen - 1].value : screenMappings.back().value;
    }

    String key = "screen" + String(newScreen) + "Visible";
    while (!preferences.getBool(key.c_str(), true)) {
        currentScreen = findScreenIndexByValue(newScreen);
        if (forward) {
            newScreen = (currentScreen < screenMappings.size() - 1) ? 
                screenMappings[currentScreen + 1].value : screenMappings.front().value;
        } else {
            newScreen = (currentScreen > 0) ? 
                screenMappings[currentScreen - 1].value : screenMappings.back().value;
        }
        key = "screen" + String(newScreen) + "Visible";
    }
    
    return newScreen;
}

void ScreenHandler::nextScreen() {
    if (handleCurrencyRotation(true)) return;
    
    int currentIndex = findScreenIndexByValue(getCurrentScreen());
    int nextScreen = findNextVisibleScreen(currentIndex, true);
    
    // If moving from a currency-specific screen to another currency-specific screen
    // reset to first currency 
    // also if moving from a currency-specific screen to a non-currency-specific screen
    if (
        isCurrencySpecific(getCurrentScreen())
    ) {
        std::vector<std::string> ac = getActiveCurrencies();
        if (!ac.empty()) {
            setCurrentCurrency(getCurrencyChar(ac.front()));
        }
    }
    
    setCurrentScreen(nextScreen);
}

void ScreenHandler::previousScreen() {
    if (handleCurrencyRotation(false)) return;
    
    int currentIndex = findScreenIndexByValue(getCurrentScreen());
    int prevScreen = findNextVisibleScreen(currentIndex, false);
    
    // If moving from a currency-specific screen to another currency-specific screen
    // reset to last currency
    // also if moving from a non-currency-specific screen to a currency-specific screen
    if (isCurrencySpecific(getCurrentScreen())) {
        std::vector<std::string> ac = getActiveCurrencies();
        if (!ac.empty()) {
            setCurrentCurrency(getCurrencyChar(ac.back()));
        }
    }
    
    setCurrentScreen(prevScreen);
}

void ScreenHandler::showSystemStatusScreen() {
    std::array<String, NUM_SCREENS> sysStatusEpdContent;
    std::fill(sysStatusEpdContent.begin(), sysStatusEpdContent.end(), "");

    String ipAddr = WiFi.localIP().toString();
    String subNet = WiFi.subnetMask().toString();

    sysStatusEpdContent[0] = "IP/Subnet";

    int ipAddrPos = 0;
    int subnetPos = 0;
    for (int i = 0; i < 4; i++) {
        sysStatusEpdContent[1 + i] = ipAddr.substring(0, ipAddr.indexOf('.')) +
                                     "/" + subNet.substring(0, subNet.indexOf('.'));
        ipAddrPos = ipAddr.indexOf('.') + 1;
        subnetPos = subNet.indexOf('.') + 1;
        ipAddr = ipAddr.substring(ipAddrPos);
        subNet = subNet.substring(subnetPos);
    }
    sysStatusEpdContent[NUM_SCREENS - 2] = "RAM/Status";

    sysStatusEpdContent[NUM_SCREENS - 1] =
        String((int)round(ESP.getFreeHeap() / 1024)) + "/" +
        (int)round(ESP.getHeapSize() / 1024);
    setCurrentScreen(SCREEN_CUSTOM);
    EPDManager::getInstance().setContent(sysStatusEpdContent);
}

// Keep these as free functions
void workerTask(void *pvParameters) {
    WorkItem receivedItem;

    while (1) {
        if (xQueueReceive(workQueue, &receivedItem, portMAX_DELAY)) {
            uint currentScreenValue = ScreenHandler::getCurrentScreen();
            
            switch (receivedItem.type) {
                case TASK_BITAXE_UPDATE: {
                    if (currentScreenValue != SCREEN_BITAXE_HASHRATE && 
                        currentScreenValue != SCREEN_BITAXE_BESTDIFF) break;
                        
                    taskEpdContent = (currentScreenValue == SCREEN_BITAXE_HASHRATE) ?
                        parseBitaxeHashRate(BitaxeFetch::getInstance().getHashRate()) :
                        parseBitaxeBestDiff(BitaxeFetch::getInstance().getBestDiff());
                    EPDManager::getInstance().setContent(taskEpdContent);
                    break;
                }

                case TASK_MINING_POOL_STATS_UPDATE: {
                    if (currentScreenValue != SCREEN_MINING_POOL_STATS_HASHRATE && 
                        currentScreenValue != SCREEN_MINING_POOL_STATS_EARNINGS) break;
                        
                    taskEpdContent = (currentScreenValue == SCREEN_MINING_POOL_STATS_HASHRATE) ?
                        parseMiningPoolStatsHashRate(MiningPoolStatsFetch::getInstance().getHashRate(), *MiningPoolStatsFetch::getInstance().getPool()) :
                        parseMiningPoolStatsDailyEarnings(MiningPoolStatsFetch::getInstance().getDailyEarnings(), 
                            MiningPoolStatsFetch::getInstance().getPool()->getDailyEarningsLabel(), 
                            *MiningPoolStatsFetch::getInstance().getPool());
                    EPDManager::getInstance().setContent(taskEpdContent);
                    break;
                }

                case TASK_PRICE_UPDATE: {
                    uint currency = ScreenHandler::getCurrentCurrency();
                    uint price = getPrice(currency);

                    if (currentScreenValue == SCREEN_BTC_TICKER) {
                        taskEpdContent = parsePriceData(price, currency, preferences.getBool("suffixPrice", DEFAULT_SUFFIX_PRICE), 
                            preferences.getBool("mowMode", DEFAULT_MOW_MODE),
                            preferences.getBool("suffixShareDot", DEFAULT_SUFFIX_SHARE_DOT)
                        );
                    } else if (currentScreenValue == SCREEN_SATS_PER_CURRENCY) {
                        taskEpdContent = parseSatsPerCurrency(price, currency, preferences.getBool("useSatsSymbol", DEFAULT_USE_SATS_SYMBOL));
                    } else {
                        auto& blockNotify = BlockNotify::getInstance();
                        taskEpdContent = parseMarketCap(blockNotify.getBlockHeight(), price, currency,  preferences.getBool("mcapBigChar", DEFAULT_MCAP_BIG_CHAR));
                    }

                    EPDManager::getInstance().setContent(taskEpdContent);
                    break;
                }
                case TASK_FEE_UPDATE: {
                    if (currentScreenValue == SCREEN_BLOCK_FEE_RATE) {
                        auto& blockNotify = BlockNotify::getInstance();
                        taskEpdContent = parseBlockFees(static_cast<std::uint16_t>(blockNotify.getBlockMedianFee()));
                        EPDManager::getInstance().setContent(taskEpdContent);
                    } 
                    break;
                }
                case TASK_BLOCK_UPDATE: {
                    if (currentScreenValue != SCREEN_HALVING_COUNTDOWN) {
                        auto& blockNotify = BlockNotify::getInstance();
                        taskEpdContent = parseBlockHeight(blockNotify.getBlockHeight());
                    } else {
                        auto& blockNotify = BlockNotify::getInstance();
                        taskEpdContent = parseHalvingCountdown(blockNotify.getBlockHeight(), preferences.getBool("useBlkCountdown", DEFAULT_USE_BLOCK_COUNTDOWN));
                    }

                    if (currentScreenValue == SCREEN_HALVING_COUNTDOWN ||
                        currentScreenValue == SCREEN_BLOCK_HEIGHT) {
                        EPDManager::getInstance().setContent(taskEpdContent);
                    }
                    break;
                }
                case TASK_TIME_UPDATE: {
                    if (currentScreenValue == SCREEN_TIME) {
                        time_t currentTime;
                        struct tm timeinfo;
                        time(&currentTime);
                        localtime_r(&currentTime, &timeinfo);
                        std::string timeString;

                        String minute = String(timeinfo.tm_min);
                        if (minute.length() < 2) {
                            minute = "0" + minute;
                        }

                        timeString =
                            std::to_string(timeinfo.tm_hour) + ":" + minute.c_str();
                        timeString.insert(timeString.begin(),
                                          NUM_SCREENS - timeString.length(), ' ');
                        taskEpdContent[0] = std::to_string(timeinfo.tm_mday) + "/" +
                                            std::to_string(timeinfo.tm_mon + 1);

                        for (uint i = 1; i < NUM_SCREENS; i++) {
                            taskEpdContent[i] = timeString[i];
                        }
                        EPDManager::getInstance().setContent(taskEpdContent);
                    }

                    break;
                }
                // Add more cases for additional task types
            }
        }
    }
}

void taskScreenRotate(void *pvParameters) {
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        ScreenHandler::nextScreen();
    }
}

void setupTasks() {
    workQueue = xQueueCreate(WORK_QUEUE_SIZE, sizeof(WorkItem));
    loadStoredPrices();

    xTaskCreate(workerTask, "workerTask", 4096, NULL, tskIDLE_PRIORITY,
                &workerTaskHandle);

    xTaskCreate(taskScreenRotate, "rotateScreen", 4096, NULL, tskIDLE_PRIORITY,
                &taskScreenRotateTaskHandle);

    if (findScreenIndexByValue(preferences.getUInt("currentScreen", DEFAULT_CURRENT_SCREEN)) != -1)
        ScreenHandler::setCurrentScreen(preferences.getUInt("currentScreen", DEFAULT_CURRENT_SCREEN));
}

void cleanup() {
    vQueueDelete(workQueue);
    // Add any other cleanup needed
}