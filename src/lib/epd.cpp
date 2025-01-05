#include "epd.hpp"

// Initialize static members
#ifdef IS_BTCLOCK_REV_B
Native_Pin EPDManager::EPD_DC(14);
std::array<Native_Pin, NUM_SCREENS> EPDManager::EPD_CS = {
    Native_Pin(2), Native_Pin(4), Native_Pin(6), Native_Pin(10),
    Native_Pin(38), Native_Pin(21), Native_Pin(17)
};
std::array<Native_Pin, NUM_SCREENS> EPDManager::EPD_BUSY = {
    Native_Pin(3), Native_Pin(5), Native_Pin(7), Native_Pin(9),
    Native_Pin(37), Native_Pin(18), Native_Pin(16)
};
std::array<MCP23X17_Pin, NUM_SCREENS> EPDManager::EPD_RESET = {
    MCP23X17_Pin(mcp1, 8), MCP23X17_Pin(mcp1, 9), MCP23X17_Pin(mcp1, 10),
    MCP23X17_Pin(mcp1, 11), MCP23X17_Pin(mcp1, 12), MCP23X17_Pin(mcp1, 13),
    MCP23X17_Pin(mcp1, 14)
};
#elif defined(IS_BTCLOCK_V8)
Native_Pin EPDManager::EPD_DC(38);
std::array<MCP23X17_Pin, NUM_SCREENS> EPDManager::EPD_BUSY = {
    MCP23X17_Pin(mcp1, 8), MCP23X17_Pin(mcp1, 9), MCP23X17_Pin(mcp1, 10),
    MCP23X17_Pin(mcp1, 11), MCP23X17_Pin(mcp1, 12), MCP23X17_Pin(mcp1, 13),
    MCP23X17_Pin(mcp1, 14), MCP23X17_Pin(mcp1, 4)
};
std::array<MCP23X17_Pin, NUM_SCREENS> EPDManager::EPD_CS = {
    MCP23X17_Pin(mcp2, 8), MCP23X17_Pin(mcp2, 10), MCP23X17_Pin(mcp2, 12),
    MCP23X17_Pin(mcp2, 14), MCP23X17_Pin(mcp2, 0), MCP23X17_Pin(mcp2, 2),
    MCP23X17_Pin(mcp2, 4), MCP23X17_Pin(mcp2, 6)
};
std::array<MCP23X17_Pin, NUM_SCREENS> EPDManager::EPD_RESET = {
    MCP23X17_Pin(mcp2, 9), MCP23X17_Pin(mcp2, 11), MCP23X17_Pin(mcp2, 13),
    MCP23X17_Pin(mcp2, 15), MCP23X17_Pin(mcp2, 1), MCP23X17_Pin(mcp2, 3),
    MCP23X17_Pin(mcp2, 5), MCP23X17_Pin(mcp2, 7)
};
#else
Native_Pin EPDManager::EPD_DC(14);
std::array<Native_Pin, NUM_SCREENS> EPDManager::EPD_CS = {
    Native_Pin(2), Native_Pin(4), Native_Pin(6), Native_Pin(10),
    Native_Pin(33), Native_Pin(21), Native_Pin(17)
};
std::array<Native_Pin, NUM_SCREENS> EPDManager::EPD_BUSY = {
    Native_Pin(3), Native_Pin(5), Native_Pin(7), Native_Pin(9),
    Native_Pin(37), Native_Pin(18), Native_Pin(16)
};
std::array<MCP23X17_Pin, NUM_SCREENS> EPDManager::EPD_RESET = {
    MCP23X17_Pin(mcp1, 8), MCP23X17_Pin(mcp1, 9), MCP23X17_Pin(mcp1, 10),
    MCP23X17_Pin(mcp1, 11), MCP23X17_Pin(mcp1, 12), MCP23X17_Pin(mcp1, 13),
    MCP23X17_Pin(mcp1, 14)
};
#endif

EPDManager& EPDManager::getInstance() {
    static EPDManager instance;
    return instance;
}

EPDManager::EPDManager() 
    : currentContent{}
    , content{}
    , lastFullRefresh{}
    , tasks{}
    , updateQueue{nullptr}
    , antonioFonts{nullptr, nullptr, nullptr}
    , oswaldFonts{nullptr, nullptr, nullptr}
    , fontSmall{nullptr}
    , fontBig{nullptr}
    , fontMedium{nullptr}
    , fontSatsymbol{nullptr}
    , bgColor{GxEPD_BLACK}
    , fgColor{GxEPD_WHITE}
    , displays{
        #ifdef IS_BTCLOCK_V8
        EPD_CLASS(&EPD_CS[0], &EPD_DC, &EPD_RESET[0], &EPD_BUSY[0]),
            EPD_CLASS(&EPD_CS[1], &EPD_DC, &EPD_RESET[1], &EPD_BUSY[1]),
            EPD_CLASS(&EPD_CS[2], &EPD_DC, &EPD_RESET[2], &EPD_BUSY[2]),
            EPD_CLASS(&EPD_CS[3], &EPD_DC, &EPD_RESET[3], &EPD_BUSY[3]),
            EPD_CLASS(&EPD_CS[4], &EPD_DC, &EPD_RESET[4], &EPD_BUSY[4]),
            EPD_CLASS(&EPD_CS[5], &EPD_DC, &EPD_RESET[5], &EPD_BUSY[5]),
            EPD_CLASS(&EPD_CS[6], &EPD_DC, &EPD_RESET[6], &EPD_BUSY[6]),
            EPD_CLASS(&EPD_CS[7], &EPD_DC, &EPD_RESET[7], &EPD_BUSY[7])
        #else
            EPD_CLASS(&EPD_CS[0], &EPD_DC, &EPD_RESET[0], &EPD_BUSY[0]),
            EPD_CLASS(&EPD_CS[1], &EPD_DC, &EPD_RESET[1], &EPD_BUSY[1]),
            EPD_CLASS(&EPD_CS[2], &EPD_DC, &EPD_RESET[2], &EPD_BUSY[2]),
            EPD_CLASS(&EPD_CS[3], &EPD_DC, &EPD_RESET[3], &EPD_BUSY[3]),
            EPD_CLASS(&EPD_CS[4], &EPD_DC, &EPD_RESET[4], &EPD_BUSY[4]),
            EPD_CLASS(&EPD_CS[5], &EPD_DC, &EPD_RESET[5], &EPD_BUSY[5]),
            EPD_CLASS(&EPD_CS[6], &EPD_DC, &EPD_RESET[6], &EPD_BUSY[6])
        #endif
    }
{
}

EPDManager::~EPDManager() {
    // Clean up tasks
    for (auto& task : tasks) {
        if (task != nullptr) {
            vTaskDelete(task);
        }
    }

    // Clean up queue
    if (updateQueue != nullptr) {
        vQueueDelete(updateQueue);
    }

    // Clean up fonts
    delete antonioFonts.big;
    delete antonioFonts.medium;
    delete antonioFonts.small;
    delete oswaldFonts.big;
    delete oswaldFonts.medium;
    delete oswaldFonts.small;
}

void EPDManager::initialize() {
    // Load fonts based on preference
    String fontName = preferences.getString("fontName", DEFAULT_FONT_NAME);
    loadFonts(fontName);

    // Initialize displays
    std::lock_guard<std::mutex> lockMcp(mcpMutex);
    for (auto& display : displays) {
        display.init(0, true, 30);
    }

    // Create update queue and task
    updateQueue = xQueueCreate(UPDATE_QUEUE_SIZE, sizeof(UpdateDisplayTaskItem));
    xTaskCreate(prepareDisplayUpdateTask, "PrepareUpd", EPD_TASK_STACK_SIZE * 2, nullptr, 11, nullptr);

    // Create display update tasks
    for (size_t i = 0; i < NUM_SCREENS; i++) {
        auto* taskParam = new int(i);
        xTaskCreate(updateDisplayTask, ("EpdUpd" + String(i)).c_str(), EPD_TASK_STACK_SIZE, 
                   taskParam, 11, &tasks[i]);
    }

    // Check for storage mode (prevents burn-in)
    if (mcp1.read1(0) == LOW) {
        setForegroundColor(GxEPD_BLACK);
        setBackgroundColor(GxEPD_WHITE);
        content.fill("");
    } else {
        // Initialize with custom text or default
        String customText = preferences.getString("displayText", DEFAULT_BOOT_TEXT);
        std::array<String, NUM_SCREENS> newContent;
        newContent.fill(" ");
        
        for (size_t i = 0; i < std::min(customText.length(), (size_t)NUM_SCREENS); i++) {
            newContent[i] = String(customText[i]);
        }
        
        content = newContent;
    }

    setContent(content);
}

void EPDManager::loadFonts(const String& fontName) {
    if (fontName == FontNames::ANTONIO) {
        // Load Antonio fonts
        antonioFonts.big = FontLoader::loadCompressedFont(Antonio_SemiBold90pt7b_Properties);
        antonioFonts.medium = FontLoader::loadCompressedFont(Antonio_SemiBold40pt7b_Properties);
        antonioFonts.small = FontLoader::loadCompressedFont(Antonio_SemiBold20pt7b_Properties);

        fontBig = antonioFonts.big;
        fontMedium = antonioFonts.medium;
        fontSmall = antonioFonts.small;
    } else if (fontName == FontNames::OSWALD) {
        // Load Oswald fonts
        oswaldFonts.big = FontLoader::loadCompressedFont(Oswald_Medium80pt7b_Properties);
        oswaldFonts.medium = FontLoader::loadCompressedFont(Oswald_Medium30pt7b_Properties);
        oswaldFonts.small = FontLoader::loadCompressedFont(Oswald_Medium20pt7b_Properties);

        fontBig = oswaldFonts.big;
        fontMedium = oswaldFonts.medium;
        fontSmall = oswaldFonts.small;
    }

    fontSatsymbol = FontLoader::loadCompressedFont(Satoshi_Symbol90pt7b_Properties);
}

void EPDManager::forceFullRefresh() {
    std::fill(lastFullRefresh.begin(), lastFullRefresh.end(), 0);
}

void EPDManager::setContent(const std::array<String, NUM_SCREENS>& newContent, bool forceUpdate) {
    std::lock_guard<std::mutex> lock(updateMutex);
    waitUntilNoneBusy();

    for (size_t i = 0; i < NUM_SCREENS; i++) {
        if (newContent[i].compareTo(currentContent[i]) != 0 || forceUpdate) {
            content[i] = newContent[i];
            UpdateDisplayTaskItem dispUpdate{static_cast<char>(i)};
            xQueueSend(updateQueue, &dispUpdate, portMAX_DELAY);
        }
    }
}

void EPDManager::setContent(const std::array<std::string, NUM_SCREENS>& newContent) {
    std::array<String, NUM_SCREENS> conv;
    for (size_t i = 0; i < newContent.size(); ++i) {
        conv[i] = String(newContent[i].c_str());
    }
    setContent(conv);
}

std::array<String, NUM_SCREENS> EPDManager::getCurrentContent() const {
    return currentContent;
}

void EPDManager::waitUntilNoneBusy() {
    for (size_t i = 0; i < NUM_SCREENS; i++) {
        uint32_t count = 0;
        while (EPD_BUSY[i].digitalRead()) {
            count++;
            vTaskDelay(BUSY_RETRY_DELAY);

            if (count == BUSY_TIMEOUT_COUNT) {
                vTaskDelay(pdMS_TO_TICKS(100));
            } else if (count > BUSY_TIMEOUT_COUNT + 5) {
                log_e("Display %d busy timeout", i);
                break;
            }
        }
    }
}

void EPDManager::setupDisplay(uint dispNum, const GFXfont* font) {
    displays[dispNum].setRotation(2);
    displays[dispNum].setFont(font);
    displays[dispNum].setTextColor(fgColor);
    displays[dispNum].fillScreen(bgColor);
}

void EPDManager::splitText(uint dispNum, const String& top, const String& bottom, bool partial) {
    if (preferences.getBool("verticalDesc", DEFAULT_VERTICAL_DESC) && dispNum == 0) {
        displays[dispNum].setRotation(1);
    } else {
        displays[dispNum].setRotation(2);
    }
    displays[dispNum].setFont(fontSmall);
    displays[dispNum].setTextColor(fgColor);

    // Top text
    int16_t ttbx, ttby;
    uint16_t ttbw, ttbh;
    displays[dispNum].getTextBounds(top, 0, 0, &ttbx, &ttby, &ttbw, &ttbh);
    uint16_t tx = ((displays[dispNum].width() - ttbw) / 2) - ttbx;
    uint16_t ty = ((displays[dispNum].height() - ttbh) / 2) - ttby - ttbh / 2 - 12;

    // Bottom text
    int16_t tbbx, tbby;
    uint16_t tbbw, tbbh;
    displays[dispNum].getTextBounds(bottom, 0, 0, &tbbx, &tbby, &tbbw, &tbbh);
    uint16_t bx = ((displays[dispNum].width() - tbbw) / 2) - tbbx;
    uint16_t by = ((displays[dispNum].height() - tbbh) / 2) - tbby + tbbh / 2 + 12;

    // Make separator as wide as the shortest text
    uint16_t lineWidth = (tbbw < ttbh) ? tbbw : ttbw;
    uint16_t lineX = round((displays[dispNum].width() - lineWidth) / 2);

    displays[dispNum].fillScreen(bgColor);
    displays[dispNum].setCursor(tx, ty);
    displays[dispNum].print(top);
    displays[dispNum].fillRoundRect(lineX, displays[dispNum].height() / 2 - 3,
                                  lineWidth, 6, 3, fgColor);
    displays[dispNum].setCursor(bx, by);
    displays[dispNum].print(bottom);
}

void EPDManager::showDigit(uint dispNum, char chr, bool partial, const GFXfont* font) {
    String str(chr);
    if (chr == '.') {
        str = "!";
    }

    setupDisplay(dispNum, font);

    int16_t tbx, tby;
    uint16_t tbw, tbh;
    displays[dispNum].getTextBounds(str, 0, 0, &tbx, &tby, &tbw, &tbh);

    uint16_t x = ((displays[dispNum].width() - tbw) / 2) - tbx;
    uint16_t y = ((displays[dispNum].height() - tbh) / 2) - tby;

    displays[dispNum].setCursor(x, y);
    displays[dispNum].print(str);

    if (chr == '.') {
        displays[dispNum].fillRect(0, 0, displays[dispNum].width(),
                               round(displays[dispNum].height() * 0.67), bgColor);
    }
}

void EPDManager::showChars(uint dispNum, const String& chars, bool partial, const GFXfont* font) {
    setupDisplay(dispNum, font);

    int16_t tbx, tby;
    uint16_t tbw, tbh;
    displays[dispNum].getTextBounds(chars, 0, 0, &tbx, &tby, &tbw, &tbh);

    // Center the bounding box by transposition of the origin
    uint16_t x = ((displays[dispNum].width() - tbw) / 2) - tbx;
    uint16_t y = ((displays[dispNum].height() - tbh) / 2) - tby;

    for (size_t i = 0; i < chars.length(); i++) {
        char c = chars[i];
        if (c == '.' || c == ',') {
            // For the dot, calculate its specific descent
            GFXglyph* dotGlyph = &font->glyph[c - font->first];
            int16_t dotDescent = dotGlyph->yOffset;

            // Draw the dot with adjusted y-position
            displays[dispNum].setCursor(x, y + dotDescent + dotGlyph->height + 8);
            displays[dispNum].print(c);
        } else {
            // For other characters, use the original y-position
            displays[dispNum].setCursor(x, y);
            displays[dispNum].print(c);
        }

        // Move x-position for the next character
        x += font->glyph[c - font->first].xAdvance;
    }
}

bool EPDManager::renderIcon(uint dispNum, const String& text, bool partial) {
    displays[dispNum].setRotation(2);
    displays[dispNum].setPartialWindow(0, 0, displays[dispNum].width(),
                                   displays[dispNum].height());
    displays[dispNum].fillScreen(bgColor);
    displays[dispNum].setTextColor(fgColor);

    uint iconIndex = 0;
    uint width = 122;
    uint height = 122;

    if (text.endsWith("rocket")) {
        iconIndex = 1;
    } else if (text.endsWith("lnbolt")) {
        iconIndex = 2;
    } else if (text.endsWith("bitaxe")) {
        width = 88;
        height = 220;
        iconIndex = 3;
    } else if (text.endsWith("miningpool")) {
        LogoData logo = MiningPoolStatsFetch::getInstance().getLogo();
        if (logo.size == 0) {
            Serial.println(F("No logo found"));
            return false;
        }

        int x_offset = (displays[dispNum].width() - logo.width) / 2;
        int y_offset = (displays[dispNum].height() - logo.height) / 2;
        displays[dispNum].drawInvertedBitmap(x_offset, y_offset, logo.data, 
                                         logo.width, logo.height, fgColor);
        return true;
    }

    int x_offset = (displays[dispNum].width() - width) / 2;
    int y_offset = (displays[dispNum].height() - height) / 2;
    displays[dispNum].drawInvertedBitmap(x_offset, y_offset, epd_icons_allArray[iconIndex], 
                                     width, height, fgColor);
    return true;
}

void EPDManager::renderText(uint dispNum, const String& text, bool partial) {
    displays[dispNum].setRotation(2);
    displays[dispNum].setPartialWindow(0, 0, displays[dispNum].width(),
                                   displays[dispNum].height());
    displays[dispNum].fillScreen(GxEPD_WHITE);
    displays[dispNum].setTextColor(GxEPD_BLACK);
    displays[dispNum].setCursor(0, 50);

    std::stringstream ss;
    ss.str(text.c_str());
    std::string line;

    while (std::getline(ss, line, '\n')) {
        if (line.rfind("*", 0) == 0) {
            line.erase(std::remove(line.begin(), line.end(), '*'), line.end());
            displays[dispNum].setFont(&FreeSansBold9pt7b);
        } else {
            displays[dispNum].setFont(&FreeSans9pt7b);
        }
        displays[dispNum].println(line.c_str());
    }
}

void EPDManager::renderQr(uint dispNum, const String& text, bool partial) {
#ifdef USE_QR
    // Dynamically allocate QR buffer
    uint8_t* qrcode = (uint8_t*)malloc(qrcodegen_BUFFER_LEN_MAX);
    if (!qrcode) {
        log_e("Failed to allocate QR buffer");
        return;
    }

    uint8_t tempBuffer[800];
    bool ok = qrcodegen_encodeText(
        text.substring(2).c_str(), tempBuffer, qrcode, qrcodegen_Ecc_LOW,
        qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, true);

    if (ok) {
        const int size = qrcodegen_getSize(qrcode);
        const int padding = floor(float(displays[dispNum].width() - (size * 4)) / 2);
        const int paddingY = floor(float(displays[dispNum].height() - (size * 4)) / 2);
        
        displays[dispNum].setRotation(2);
        displays[dispNum].setPartialWindow(0, 0, displays[dispNum].width(),
                                       displays[dispNum].height());
        displays[dispNum].fillScreen(GxEPD_WHITE);

        for (int y = 0; y < size * 4; y++) {
            for (int x = 0; x < size * 4; x++) {
                displays[dispNum].drawPixel(
                    padding + x, paddingY + y,
                    qrcodegen_getModule(qrcode, floor(float(x) / 4), floor(float(y) / 4))
                        ? GxEPD_BLACK
                        : GxEPD_WHITE);
            }
        }
    }

    free(qrcode);
#endif
}

int16_t EPDManager::calculateDescent(const GFXfont* font) {
    int16_t maxDescent = 0;
    for (uint16_t i = font->first; i <= font->last; i++) {
        GFXglyph* glyph = &font->glyph[i - font->first];
        int16_t descent = glyph->yOffset;
        if (descent > maxDescent) {
            maxDescent = descent;
        }
    }
    return maxDescent;
}

void EPDManager::updateDisplayTask(void* pvParameters) noexcept {
    auto& instance = EPDManager::getInstance();
    const int epdIndex = *(int*)pvParameters;
    delete (int*)pvParameters;

    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        std::lock_guard<std::mutex> lock(instance.displayMutexes[epdIndex]);
        {
            std::lock_guard<std::mutex> lockMcp(mcpMutex);
            instance.displays[epdIndex].init(0, false, 40);
        }

        uint32_t count = 0;
        while (instance.EPD_BUSY[epdIndex].digitalRead() == HIGH || count < 10) {
            vTaskDelay(pdMS_TO_TICKS(100));
            count++;
        }

        bool updatePartial = true;
        if (!instance.lastFullRefresh[epdIndex] ||
            (millis() - instance.lastFullRefresh[epdIndex]) >
            (preferences.getUInt("fullRefreshMin", DEFAULT_MINUTES_FULL_REFRESH) * 60 * 1000)) {
            updatePartial = false;
        }

        char tries = 0;
        while (tries < 3) {
            if (instance.displays[epdIndex].displayWithReturn(updatePartial)) {
                instance.displays[epdIndex].powerOff();
                instance.currentContent[epdIndex] = instance.content[epdIndex];
                if (!updatePartial) {
                    instance.lastFullRefresh[epdIndex] = millis();
                }

                if (eventSourceTaskHandle != nullptr) {
                    xTaskNotifyGive(eventSourceTaskHandle);
                }
                break;
            }

            vTaskDelay(pdMS_TO_TICKS(100));
            tries++;
        }
    }
}

void EPDManager::prepareDisplayUpdateTask(void* pvParameters) {
    auto& instance = EPDManager::getInstance();
    UpdateDisplayTaskItem receivedItem;

    for (;;) {
        if (xQueueReceive(instance.updateQueue, &receivedItem, portMAX_DELAY)) {
            uint epdIndex = receivedItem.dispNum;
            std::lock_guard<std::mutex> lock(instance.displayMutexes[epdIndex]);

            bool updatePartial = true;

            if (instance.content[epdIndex].length() > 1 && 
                strstr(instance.content[epdIndex].c_str(), "/") != nullptr) {
                String top = instance.content[epdIndex].substring(
                    0, instance.content[epdIndex].indexOf("/"));
                String bottom = instance.content[epdIndex].substring(
                    instance.content[epdIndex].indexOf("/") + 1);
                instance.splitText(epdIndex, top, bottom, updatePartial);
            } else if (instance.content[epdIndex].startsWith(F("qr"))) {
                instance.renderQr(epdIndex, instance.content[epdIndex], updatePartial);
            } else if (instance.content[epdIndex].startsWith(F("mdi"))) {
                if (!instance.renderIcon(epdIndex, instance.content[epdIndex], updatePartial)) {
                    continue;
                }
            } else if (instance.content[epdIndex].length() > 5) {
                instance.renderText(epdIndex, instance.content[epdIndex], updatePartial);
            } else {
                if (instance.content[epdIndex].length() == 2) {
                    instance.showChars(epdIndex, instance.content[epdIndex], updatePartial, instance.fontBig);
                } else if (instance.content[epdIndex].length() > 1 && 
                         instance.content[epdIndex].indexOf(".") == -1) {
                    if (instance.content[epdIndex].equals("STS")) {
                        instance.showDigit(epdIndex, 'S', updatePartial, instance.fontSatsymbol);
                    } else {
                        instance.showChars(epdIndex, instance.content[epdIndex], updatePartial,
                                       instance.fontMedium);
                    }
                } else {
                    instance.showDigit(epdIndex, instance.content[epdIndex].c_str()[0], 
                                   updatePartial, instance.fontBig);
                }
            }

            xTaskNotifyGive(instance.tasks[epdIndex]);
        }
    }
}