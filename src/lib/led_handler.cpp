#include "led_handler.hpp"

// Singleton instance
LedHandler& LedHandler::getInstance() {
    static LedHandler instance;
    return instance;
}

LedHandler::LedHandler()
    : pixels(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800)
    , ledTaskHandle(nullptr)
    , ledTaskQueue(nullptr)
    , ledTaskParams(0)
    , dndEnabled(false)
    , dndTimeBasedEnabled(false)
    , dndTimeRange{23, 0, 7, 0}  // Default: 23:00 to 07:00
#ifdef HAS_FRONTLIGHT
    , frontlightOn(false)
    , flInTransition(false)
#endif
{
}

void LedHandler::setup() {
    loadDNDSettings();
    pixels.begin();
    pixels.setBrightness(preferences.getUInt("ledBrightness", DEFAULT_LED_BRIGHTNESS));
    pixels.clear();
    pixels.show();
    setupTask();
    
    if (preferences.getBool("ledTestOnPower", DEFAULT_LED_TEST_ON_POWER)) {
        while (!ledTaskQueue) {
            delay(1);
        }
        queueEffect(LED_POWER_TEST);
    }
}

void LedHandler::setupTask() {
    ledTaskQueue = xQueueCreate(5, sizeof(uint));
    xTaskCreate(ledTask, "LedTask", 2048, this, 10, &ledTaskHandle);
}

void LedHandler::ledTask(void* pvParameters) {
    auto* handler = static_cast<LedHandler*>(pvParameters);
    while (true) {
        if (handler->ledTaskQueue != nullptr) {
            if (xQueueReceive(handler->ledTaskQueue, &handler->ledTaskParams, portMAX_DELAY) == pdPASS) {
                if (preferences.getBool("disableLeds", DEFAULT_DISABLE_LEDS)) {
                    continue;
                }

                std::array<uint32_t, NEOPIXEL_COUNT> oldLights;
                for (int i = 0; i < NEOPIXEL_COUNT; i++) {
                    oldLights[i] = handler->pixels.getPixelColor(i);
                }

#ifdef HAS_FRONTLIGHT
                uint flDelayTime = preferences.getUInt("flEffectDelay");
#endif

                switch (handler->ledTaskParams) {
                    case LED_POWER_TEST:
#ifdef HAS_FRONTLIGHT
                        handler->frontlightFadeInAll(preferences.getUInt("flEffectDelay"), true);
#endif
                        handler->rainbow(20);
                        handler->pixels.clear();
                        break;

                    case LED_EFFECT_WIFI_CONNECT_ERROR:
                        handler->blinkDelayTwoColor(100, 3, handler->pixels.Color(8, 161, 236),
                                                  handler->pixels.Color(255, 0, 0));
                        break;

                    case LED_EFFECT_CONFIGURING:
                        for (int i = NEOPIXEL_COUNT; i--; i > 0) {
                            for (int j = NEOPIXEL_COUNT; j--; j > 0) {
                                uint32_t c = handler->pixels.Color(0, 0, 0);
                                if (i == j)
                                    c = handler->pixels.Color(0, 0, 255);
                                handler->pixels.setPixelColor(j, c);
                            }
                            handler->pixels.show();
                            delay(100);
                        }
                        handler->pixels.clear();
                        handler->pixels.show();
                        break;

                    case LED_FLASH_ERROR:
                        handler->blinkDelayColor(250, 3, 255, 0, 0);
                        break;

                    case LED_EFFECT_HEARTBEAT:
                        handler->blinkDelayColor(150, 2, 0, 0, 255);
                        break;

                    case LED_DATA_BLOCK_ERROR:
                        handler->blinkDelayColor(150, 2, 128, 0, 128);
                        break;

                    case LED_DATA_PRICE_ERROR:
                        handler->blinkDelayColor(150, 2, 177, 90, 31);
                        break;

                    case LED_FLASH_IDENTIFY:
                        handler->blinkDelayTwoColor(100, 2, handler->pixels.Color(255, 0, 0),
                                                  handler->pixels.Color(0, 255, 255));
                        handler->blinkDelayTwoColor(100, 2, handler->pixels.Color(0, 255, 0),
                                                  handler->pixels.Color(0, 0, 255));
                        break;

                    case LED_EFFECT_WIFI_CONNECT_SUCCESS:
                    case LED_FLASH_SUCCESS:
                        handler->blinkDelayColor(150, 3, 0, 255, 0);
                        break;

                    case LED_PROGRESS_100:
                        handler->pixels.setPixelColor(0, handler->pixels.Color(0, 255, 0));
                        [[fallthrough]];
                    case LED_PROGRESS_75:
                        handler->pixels.setPixelColor(1, handler->pixels.Color(0, 255, 0));
                        [[fallthrough]];
                    case LED_PROGRESS_50:
                        handler->pixels.setPixelColor(2, handler->pixels.Color(0, 255, 0));
                        [[fallthrough]];
                    case LED_PROGRESS_25:
                        handler->pixels.setPixelColor(3, handler->pixels.Color(0, 255, 0));
                        handler->pixels.show();
                        break;

                    case LED_EFFECT_NOSTR_ZAP:
                    {
#ifdef HAS_FRONTLIGHT
                        bool frontlightWasOn = false;
                        if (preferences.getBool("flFlashOnZap", DEFAULT_FL_FLASH_ON_ZAP)) {
                            if (handler->frontlightOn) {
                                frontlightWasOn = true;
                                handler->frontlightFadeOutAll(flDelayTime, true);
                            } else {
                                handler->frontlightFadeInAll(flDelayTime, true);
                            }

                            handler->flInTransition = true;
                        }
#endif
                        for (int flash = 0; flash < random(7, 10); flash++) {
                            handler->lightningStrike();
                            delay(random(50, 150));
                        }
#ifdef HAS_FRONTLIGHT
                        if (preferences.getBool("flFlashOnZap", DEFAULT_FL_FLASH_ON_ZAP)) {
                            handler->flInTransition = false;

                            vTaskDelay(pdMS_TO_TICKS(10));
                            if (frontlightWasOn) {
                                handler->frontlightFadeInAll(flDelayTime, true);
                            } else {
                                handler->frontlightFadeOutAll(flDelayTime, true);
                            }
                        }
#endif
                        break;
                    }

                    case LED_FLASH_UPDATE:
                        handler->blinkDelayTwoColor(250, 3, handler->pixels.Color(0, 230, 0),
                                                  handler->pixels.Color(230, 230, 0));
                        break;

                    case LED_FLASH_BLOCK_NOTIFY:
                    {
#ifdef HAS_FRONTLIGHT
                        bool frontlightWasOn = false;
                        if (preferences.getBool("flFlashOnUpd", DEFAULT_FL_FLASH_ON_UPDATE)) {
                            if (handler->frontlightOn) {
                                frontlightWasOn = true;
                                handler->frontlightFadeOutAll(flDelayTime, true);
                            } else {
                                handler->frontlightFadeInAll(flDelayTime, true);
                            }
                            handler->flInTransition = true;
                        }
#endif
                        handler->blinkDelayTwoColor(250, 3, preferences.getUInt("blockFlashColor", DEFAULT_BLOCK_FLASH_COLOR),
                                                  handler->pixels.Color(8, 2, 0));
#ifdef HAS_FRONTLIGHT
                        if (preferences.getBool("flFlashOnUpd", DEFAULT_FL_FLASH_ON_UPDATE)) {
                            vTaskDelay(pdMS_TO_TICKS(10));
                            handler->flInTransition = false;

                            if (frontlightWasOn) {
                                handler->frontlightFadeInAll(flDelayTime, true);
                            } else {
                                handler->frontlightFadeOutAll(flDelayTime, true);
                            }
                        }
#endif
                        break;
                    }

                    case LED_EFFECT_WIFI_WAIT_FOR_CONFIG:
                        handler->blinkDelayTwoColor(100, 1, handler->pixels.Color(8, 161, 236),
                                                  handler->pixels.Color(156, 225, 240));
                        break;

                    case LED_EFFECT_WIFI_ERASE_SETTINGS:
                        handler->blinkDelay(100, 3);
                        break;

                    case LED_EFFECT_WIFI_CONNECTING:
                        for (int i = NEOPIXEL_COUNT; i >= 0; i--) {
                            for (int j = NEOPIXEL_COUNT; j >= 0; j--) {
                                if (j == i) {
                                    handler->pixels.setPixelColor(i, handler->pixels.Color(16, 197, 236));
                                } else {
                                    handler->pixels.setPixelColor(j, handler->pixels.Color(0, 0, 0));
                                }
                            }
                            handler->pixels.show();
                            vTaskDelay(pdMS_TO_TICKS(100));
                        }
                        break;

                    case LED_EFFECT_PAUSE_TIMER:
                        for (int i = NEOPIXEL_COUNT; i >= 0; i--) {
                            for (int j = NEOPIXEL_COUNT; j >= 0; j--) {
                                uint32_t c = handler->pixels.Color(0, 0, 0);
                                if (i == j)
                                    c = handler->pixels.Color(0, 255, 0);
                                handler->pixels.setPixelColor(j, c);
                            }
                            handler->pixels.show();
                            delay(100);
                        }
                        handler->pixels.setPixelColor(0, handler->pixels.Color(255, 0, 0));
                        handler->pixels.show();
                        delay(900);
                        handler->pixels.clear();
                        handler->pixels.show();
                        break;

                    case LED_EFFECT_START_TIMER:
                        handler->pixels.clear();
                        handler->pixels.setPixelColor((NEOPIXEL_COUNT - 1), handler->pixels.Color(255, 0, 0));
                        handler->pixels.show();
                        delay(900);
                        for (int i = NEOPIXEL_COUNT; i--; i > 0) {
                            for (int j = NEOPIXEL_COUNT; j--; j > 0) {
                                uint32_t c = handler->pixels.Color(0, 0, 0);
                                if (i == j)
                                    c = handler->pixels.Color(0, 255, 0);
                                handler->pixels.setPixelColor(j, c);
                            }
                            handler->pixels.show();
                            delay(100);
                        }
                        handler->pixels.clear();
                        handler->pixels.show();
                        break;
                }

                // Restore previous state unless power test
                for (int i = 0; i < NEOPIXEL_COUNT; i++) {
                    handler->pixels.setPixelColor(i, oldLights[i]);
                }
                handler->pixels.show();
            }
        }
    }
}

bool LedHandler::queueEffect(uint effect) {
    if (isDNDActive()) {
        return false;
    }
    if (ledTaskQueue == nullptr) {
        return false;
    }
    xQueueSend(ledTaskQueue, &effect, portMAX_DELAY);
    return true;
}

void LedHandler::clear() {
    preferences.putBool("ledStatus", false);
    pixels.clear();
    pixels.show();
}

void LedHandler::setLights(int r, int g, int b) {
    setLights(pixels.Color(r, g, b));
}

void LedHandler::setLights(uint32_t color) {
    bool ledStatus = true;
    for (int i = 0; i < NEOPIXEL_COUNT; i++) {
        pixels.setPixelColor(i, color);
    }
    pixels.show();

    if (color == pixels.Color(0, 0, 0)) {
        ledStatus = false;
    } else {
        saveLedState();
    }
    preferences.putBool("ledStatus", ledStatus);
}

void LedHandler::saveLedState() {
    for (int i = 0; i < pixels.numPixels(); i++) {
        int pixelColor = pixels.getPixelColor(i);
        char key[12];
        snprintf(key, 12, "%s%d", "ledColor_", i);
        preferences.putUInt(key, pixelColor);
    }
    xTaskNotifyGive(eventSourceTaskHandle);
}

void LedHandler::restoreLedState() {
    for (int i = 0; i < pixels.numPixels(); i++) {
        char key[12];
        snprintf(key, 12, "%s%d", "ledColor_", i);
        uint pixelColor = preferences.getUInt(key, pixels.Color(0, 0, 0));
        pixels.setPixelColor(i, pixelColor);
    }
    pixels.show();
}

void LedHandler::rainbow(int wait) {
    for (long firstPixelHue = 0; firstPixelHue < 5 * 65536; firstPixelHue += 256) {
        pixels.rainbow(firstPixelHue);
        pixels.show();
        delayMicroseconds(wait);
    }
}

void LedHandler::theaterChase(uint32_t color, int wait) {
    for (int a = 0; a < 10; a++) {
        for (int b = 0; b < 3; b++) {
            pixels.clear();
            for (int c = b; c < pixels.numPixels(); c += 3) {
                pixels.setPixelColor(c, color);
            }
            pixels.show();
            vTaskDelay(pdMS_TO_TICKS(wait));
        }
    }
}

void LedHandler::theaterChaseRainbow(int wait) {
    int firstPixelHue = 0;
    for (int a = 0; a < 30; a++) {
        for (int b = 0; b < 3; b++) {
            pixels.clear();
            for (int c = b; c < pixels.numPixels(); c += 3) {
                int hue = firstPixelHue + c * 65536L / pixels.numPixels();
                uint32_t color = pixels.gamma32(pixels.ColorHSV(hue));
                pixels.setPixelColor(c, color);
            }
            pixels.show();
            vTaskDelay(pdMS_TO_TICKS(wait));
            firstPixelHue += 65536 / 90;
        }
    }
}

void LedHandler::lightningStrike() {
    uint32_t PURPLE = pixels.Color(128, 0, 128);
    uint32_t YELLOW = pixels.Color(255, 226, 41);

    for (int i = 0; i < pixels.numPixels(); i++) {
        pixels.setPixelColor(i, random(2) == 0 ? YELLOW : PURPLE);
    }
    pixels.show();
    delay(random(10, 50));
}

void LedHandler::blinkDelay(int d, int times) {
    for (int j = 0; j < times; j++) {
        pixels.setPixelColor(0, pixels.Color(255, 0, 0));
        pixels.setPixelColor(1, pixels.Color(0, 255, 0));
        pixels.setPixelColor(2, pixels.Color(255, 0, 0));
        pixels.setPixelColor(3, pixels.Color(0, 255, 0));
        pixels.show();
        vTaskDelay(pdMS_TO_TICKS(d));

        pixels.setPixelColor(0, pixels.Color(255, 255, 0));
        pixels.setPixelColor(1, pixels.Color(0, 255, 255));
        pixels.setPixelColor(2, pixels.Color(255, 255, 0));
        pixels.setPixelColor(3, pixels.Color(0, 255, 255));
        pixels.show();
        vTaskDelay(pdMS_TO_TICKS(d));
    }
    pixels.clear();
    pixels.show();
}

void LedHandler::blinkDelayColor(int d, int times, uint r, uint g, uint b) {
    for (int j = 0; j < times; j++) {
        for (int i = 0; i < NEOPIXEL_COUNT; i++) {
            pixels.setPixelColor(i, pixels.Color(r, g, b));
        }
        pixels.show();
        vTaskDelay(pdMS_TO_TICKS(d));

        pixels.clear();
        pixels.show();
        vTaskDelay(pdMS_TO_TICKS(d));
    }
    pixels.clear();
    pixels.show();
}

void LedHandler::blinkDelayTwoColor(int d, int times, const uint32_t& c1, const uint32_t& c2) {
    for (int j = 0; j < times; j++) {
        for (int i = 0; i < NEOPIXEL_COUNT; i++) {
            pixels.setPixelColor(i, c1);
        }
        pixels.show();
        vTaskDelay(pdMS_TO_TICKS(d));

        for (int i = 0; i < NEOPIXEL_COUNT; i++) {
            pixels.setPixelColor(i, c2);
        }
        pixels.show();
        vTaskDelay(pdMS_TO_TICKS(d));
    }
    pixels.clear();
    pixels.show();
}

// DND Implementation
void LedHandler::loadDNDSettings() {
    dndEnabled = preferences.getBool("dndEnabled", false);
    dndTimeBasedEnabled = preferences.getBool("dndTimeEnabled", false);
    
    dndTimeRange.startHour = preferences.getUChar("dndStartHour", 23);
    dndTimeRange.startMinute = preferences.getUChar("dndStartMin", 0);
    dndTimeRange.endHour = preferences.getUChar("dndEndHour", 7);
    dndTimeRange.endMinute = preferences.getUChar("dndEndMin", 0);
}

void LedHandler::setDNDEnabled(bool enabled) {
    dndEnabled = enabled;
    preferences.putBool("dndEnabled", enabled);
    if (enabled && isDNDActive()) {
        clear();
#ifdef HAS_FRONTLIGHT
        frontlightFadeOutAll();
#endif
    }
}

void LedHandler::setDNDTimeBasedEnabled(bool enabled) {
    dndTimeBasedEnabled = enabled;
    preferences.putBool("dndTimeEnabled", enabled);
    if (enabled && isDNDActive()) {
        clear();
#ifdef HAS_FRONTLIGHT
        frontlightFadeOutAll();
#endif
    }
}

void LedHandler::setDNDTimeRange(uint8_t startHour, uint8_t startMinute, uint8_t endHour, uint8_t endMinute) {
    dndTimeRange.startHour = startHour;
    dndTimeRange.startMinute = startMinute;
    dndTimeRange.endHour = endHour;
    dndTimeRange.endMinute = endMinute;
    
    preferences.putUChar("dndStartHour", startHour);
    preferences.putUChar("dndStartMin", startMinute);
    preferences.putUChar("dndEndHour", endHour);
    preferences.putUChar("dndEndMin", endMinute);
}

bool LedHandler::isTimeInDNDRange(uint8_t hour, uint8_t minute) const {
    uint16_t currentTime = hour * 60 + minute;
    uint16_t startTime = dndTimeRange.startHour * 60 + dndTimeRange.startMinute;
    uint16_t endTime = dndTimeRange.endHour * 60 + dndTimeRange.endMinute;
    
    if (startTime <= endTime) {
        return currentTime >= startTime && currentTime < endTime;
    } else {
        return currentTime >= startTime || currentTime < endTime;
    }
}

bool LedHandler::isDNDActive() const {
    if (dndEnabled) {
        return true;
    }
    
    if (dndTimeBasedEnabled) {
        time_t now;
        struct tm timeinfo;
        time(&now);
        localtime_r(&now, &timeinfo);
        return isTimeInDNDRange(timeinfo.tm_hour, timeinfo.tm_min);
    }
    
    return false;
}

#ifdef HAS_FRONTLIGHT
// Frontlight implementation
void LedHandler::frontlightFlash(int flDelayTime) {
    if (preferences.getBool("flDisable")) {
        return;
    }

    if (frontlightOn) {
        frontlightFadeOutAll(flDelayTime, true);
        frontlightFadeInAll(flDelayTime, true);
    } else {
        frontlightFadeInAll(flDelayTime, true);
        frontlightFadeOutAll(flDelayTime, true);
    }
}

void LedHandler::frontlightFadeInAll() {
    frontlightFadeInAll(preferences.getUInt("flEffectDelay"));
}

void LedHandler::frontlightFadeOutAll() {
    frontlightFadeOutAll(preferences.getUInt("flEffectDelay"));
}

void LedHandler::frontlightFadeIn(uint num) {
    frontlightFadeIn(num, preferences.getUInt("flEffectDelay"));
}

void LedHandler::frontlightFadeOut(uint num) {
    frontlightFadeOut(num, preferences.getUInt("flEffectDelay"));
}

void LedHandler::frontlightSetBrightness(uint brightness) {
    if (isDNDActive() || brightness > 4096) {
        return;
    }
    
    for (int ledPin = 0; ledPin <= NUM_SCREENS; ledPin++) {
        flArray.setPWM(ledPin + 1, 0, brightness);
    }
}

std::vector<uint16_t> LedHandler::frontlightGetStatus() {
    std::vector<uint16_t> statuses;
    for (int ledPin = 1; ledPin <= NUM_SCREENS; ledPin++) {
        uint16_t a = 0, b = 0;
        flArray.getPWM(ledPin + 1, &a, &b);
        statuses.push_back(round(b - a / 4096));
    }
    return statuses;
}

void LedHandler::frontlightFadeInAll(int flDelayTime, bool staggered) {
    if (preferences.getBool("flDisable") || frontlightIsOn() || flInTransition) {
        return;
    }

    flInTransition = true;
    const int maxBrightness = preferences.getUInt("flMaxBrightness");

    if (staggered) {
        int step = FL_FADE_STEP;
        int staggerDelay = flDelayTime / NUM_SCREENS;

        for (int dutyCycle = 0; dutyCycle <= maxBrightness + (NUM_SCREENS - 1) * maxBrightness / NUM_SCREENS; dutyCycle += step) {
            for (int ledPin = 0; ledPin < NUM_SCREENS; ledPin++) {
                int ledBrightness = dutyCycle - ledPin * maxBrightness / NUM_SCREENS;
                if (ledBrightness < 0) {
                    ledBrightness = 0;
                } else if (ledBrightness > maxBrightness) {
                    ledBrightness = maxBrightness;
                }
                flArray.setPWM(ledPin + 1, 0, ledBrightness);
            }
            vTaskDelay(pdMS_TO_TICKS(staggerDelay));
        }
    } else {
        for (int dutyCycle = 0; dutyCycle <= maxBrightness; dutyCycle += FL_FADE_STEP) {
            for (int ledPin = 0; ledPin <= NUM_SCREENS; ledPin++) {
                flArray.setPWM(ledPin + 1, 0, dutyCycle);
            }
            vTaskDelay(pdMS_TO_TICKS(flDelayTime));
        }
    }
    frontlightOn = true;
    flInTransition = false;
}

void LedHandler::frontlightFadeOutAll(int flDelayTime, bool staggered) {
    if (preferences.getBool("flDisable") || !frontlightIsOn() || flInTransition) {
        return;
    }

    flInTransition = true;
    if (staggered) {
        int maxBrightness = preferences.getUInt("flMaxBrightness");
        int step = FL_FADE_STEP;
        int staggerDelay = flDelayTime / NUM_SCREENS;

        for (int dutyCycle = maxBrightness; dutyCycle >= 0; dutyCycle -= step) {
            for (int ledPin = 0; ledPin < NUM_SCREENS; ledPin++) {
                int ledBrightness = dutyCycle - (NUM_SCREENS - 1 - ledPin) * maxBrightness / NUM_SCREENS;
                if (ledBrightness < 0) {
                    ledBrightness = 0;
                } else if (ledBrightness > maxBrightness) {
                    ledBrightness = maxBrightness;
                }
                flArray.setPWM(ledPin + 1, 0, ledBrightness);
            }
            vTaskDelay(pdMS_TO_TICKS(staggerDelay));
        }
    } else {
        for (int dutyCycle = preferences.getUInt("flMaxBrightness"); dutyCycle >= 0; dutyCycle -= FL_FADE_STEP) {
            for (int ledPin = 0; ledPin <= NUM_SCREENS; ledPin++) {
                flArray.setPWM(ledPin + 1, 0, dutyCycle);
            }
            vTaskDelay(pdMS_TO_TICKS(flDelayTime));
        }
    }

    flArray.allOFF();
    frontlightOn = false;
    flInTransition = false;
}

void LedHandler::frontlightFadeIn(uint num, int flDelayTime) {
    if (isDNDActive() || preferences.getBool("flDisable")) {
        return;
    }
    
    for (int dutyCycle = 0; dutyCycle <= preferences.getUInt("flMaxBrightness"); dutyCycle += 5) {
        flArray.setPWM(num + 1, 0, dutyCycle);
        vTaskDelay(pdMS_TO_TICKS(flDelayTime));
    }
}

void LedHandler::frontlightFadeOut(uint num, int flDelayTime) {
    if (isDNDActive() || preferences.getBool("flDisable") || !frontlightIsOn()) {
        return;
    }

    for (int dutyCycle = preferences.getUInt("flMaxBrightness"); dutyCycle >= 0; dutyCycle -= 5) {
        flArray.setPWM(num + 1, 0, dutyCycle);
        vTaskDelay(pdMS_TO_TICKS(flDelayTime));
    }
}

void LedHandler::initializeFrontlight() {
    if (!flArray.begin(PCA9685_MODE1_AUTOINCR | PCA9685_MODE1_ALLCALL, PCA9685_MODE2_TOTEMPOLE))
    {
        Serial.println(F("FL driver error"));
        return;
    }
    flArray.setFrequency(200);
    Serial.println(F("FL driver active"));

    if (!preferences.isKey("flMaxBrightness"))
    {
        preferences.putUInt("flMaxBrightness", DEFAULT_FL_MAX_BRIGHTNESS);
    }
    if (!preferences.isKey("flEffectDelay"))
    {
        preferences.putUInt("flEffectDelay", DEFAULT_FL_EFFECT_DELAY);
    }

    if (!preferences.isKey("flFlashOnUpd"))
    {
        preferences.putBool("flFlashOnUpd", DEFAULT_FL_FLASH_ON_UPDATE);
    }
}
#endif
