#include "epd.hpp"

#ifdef IS_BTCLOCK_REV_B
Native_Pin EPD_CS[NUM_SCREENS] = {
    Native_Pin(2),
    Native_Pin(4),
    Native_Pin(6),
    Native_Pin(10),
    Native_Pin(38),
    Native_Pin(21),
    Native_Pin(17),
};
Native_Pin EPD_BUSY[NUM_SCREENS] = {
    Native_Pin(3),
    Native_Pin(5),
    Native_Pin(7),
    Native_Pin(9),
    Native_Pin(37),
    Native_Pin(18),
    Native_Pin(16),
};
MCP23X17_Pin EPD_RESET_MPD[NUM_SCREENS] = {
    MCP23X17_Pin(mcp1, 8),
    MCP23X17_Pin(mcp1, 9),
    MCP23X17_Pin(mcp1, 10),
    MCP23X17_Pin(mcp1, 11),
    MCP23X17_Pin(mcp1, 12),
    MCP23X17_Pin(mcp1, 13),
    MCP23X17_Pin(mcp1, 14),
};

Native_Pin EPD_DC = Native_Pin(14);
#elif IS_BTCLOCK_V8
Native_Pin EPD_DC = Native_Pin(38);

MCP23X17_Pin EPD_BUSY[NUM_SCREENS] = {
    MCP23X17_Pin(mcp1, 8),
    MCP23X17_Pin(mcp1, 9),
    MCP23X17_Pin(mcp1, 10),
    MCP23X17_Pin(mcp1, 11),
    MCP23X17_Pin(mcp1, 12),
    MCP23X17_Pin(mcp1, 13),
    MCP23X17_Pin(mcp1, 14),
    MCP23X17_Pin(mcp1, 4),
};

MCP23X17_Pin EPD_CS[NUM_SCREENS] = {
    MCP23X17_Pin(mcp2, 8), MCP23X17_Pin(mcp2, 10), MCP23X17_Pin(mcp2, 12),
    MCP23X17_Pin(mcp2, 14), MCP23X17_Pin(mcp2, 0), MCP23X17_Pin(mcp2, 2),
    MCP23X17_Pin(mcp2, 4), MCP23X17_Pin(mcp2, 6)};

MCP23X17_Pin EPD_RESET_MPD[NUM_SCREENS] = {
    MCP23X17_Pin(mcp2, 9),
    MCP23X17_Pin(mcp2, 11),
    MCP23X17_Pin(mcp2, 13),
    MCP23X17_Pin(mcp2, 15),
    MCP23X17_Pin(mcp2, 1),
    MCP23X17_Pin(mcp2, 3),
    MCP23X17_Pin(mcp2, 5),
    MCP23X17_Pin(mcp2, 7),
};
#else
Native_Pin EPD_CS[NUM_SCREENS] = {
    Native_Pin(2),
    Native_Pin(4),
    Native_Pin(6),
    Native_Pin(10),
    Native_Pin(33),
    Native_Pin(21),
    Native_Pin(17),
#if NUM_SCREENS == 9
    // MCP23X17_Pin(mcp2, 7),
    Native_Pin(-1),
    Native_Pin(-1),
#endif
};
Native_Pin EPD_BUSY[NUM_SCREENS] = {
    Native_Pin(3),
    Native_Pin(5),
    Native_Pin(7),
    Native_Pin(9),
    Native_Pin(37),
    Native_Pin(18),
    Native_Pin(16),
};
MCP23X17_Pin EPD_RESET_MPD[NUM_SCREENS] = {
    MCP23X17_Pin(mcp1, 8),
    MCP23X17_Pin(mcp1, 9),
    MCP23X17_Pin(mcp1, 10),
    MCP23X17_Pin(mcp1, 11),
    MCP23X17_Pin(mcp1, 12),
    MCP23X17_Pin(mcp1, 13),
    MCP23X17_Pin(mcp1, 14),
};

Native_Pin EPD_DC = Native_Pin(14);
#endif

GxEPD2_BW<EPD_CLASS, EPD_CLASS::HEIGHT> displays[NUM_SCREENS] = {
    EPD_CLASS(&EPD_CS[0], &EPD_DC, &EPD_RESET_MPD[0], &EPD_BUSY[0]),
    EPD_CLASS(&EPD_CS[1], &EPD_DC, &EPD_RESET_MPD[1], &EPD_BUSY[1]),
    EPD_CLASS(&EPD_CS[2], &EPD_DC, &EPD_RESET_MPD[2], &EPD_BUSY[2]),
    EPD_CLASS(&EPD_CS[3], &EPD_DC, &EPD_RESET_MPD[3], &EPD_BUSY[3]),
    EPD_CLASS(&EPD_CS[4], &EPD_DC, &EPD_RESET_MPD[4], &EPD_BUSY[4]),
    EPD_CLASS(&EPD_CS[5], &EPD_DC, &EPD_RESET_MPD[5], &EPD_BUSY[5]),
    EPD_CLASS(&EPD_CS[6], &EPD_DC, &EPD_RESET_MPD[6], &EPD_BUSY[6]),
#ifdef IS_BTCLOCK_V8
    EPD_CLASS(&EPD_CS[7], &EPD_DC, &EPD_RESET_MPD[7], &EPD_BUSY[7]),
#endif
};

std::array<String, NUM_SCREENS> currentEpdContent;
std::array<String, NUM_SCREENS> epdContent;
uint32_t lastFullRefresh[NUM_SCREENS];
TaskHandle_t tasks[NUM_SCREENS];
// TaskHandle_t epdTaskHandle = NULL;

#define UPDATE_QUEUE_SIZE 14
QueueHandle_t updateQueue;

// SemaphoreHandle_t epdUpdateSemaphore[NUM_SCREENS];

int fgColor = GxEPD_WHITE;
int bgColor = GxEPD_BLACK;

struct FontFamily {
    GFXfont* big;
    GFXfont* medium;
    GFXfont* small;
};

FontFamily antonioFonts = {nullptr, nullptr, nullptr};
FontFamily oswaldFonts = {nullptr, nullptr, nullptr};

const GFXfont *FONT_SMALL;
const GFXfont *FONT_BIG;
const GFXfont *FONT_MEDIUM;
const GFXfont *FONT_SATSYMBOL;

std::mutex epdUpdateMutex;
std::mutex epdMutex[NUM_SCREENS];

uint8_t qrcode[800];

#ifdef IS_BTCLOCK_V8
#define EPD_TASK_STACK_SIZE 4096
#else
#define EPD_TASK_STACK_SIZE 2048
#endif

#define BUSY_TIMEOUT_COUNT 200
#define BUSY_RETRY_DELAY pdMS_TO_TICKS(10)

void forceFullRefresh()
{
  for (uint i = 0; i < NUM_SCREENS; i++)
  {
    lastFullRefresh[i] = NULL;
  }
}

GFXfont font90;

void loadFonts(const String& fontName) {
  if (fontName == FontNames::ANTONIO) {
    // Load Antonio fonts
    antonioFonts.big = FontLoader::loadCompressedFont(Antonio_SemiBold90pt7b_Properties);
    antonioFonts.medium = FontLoader::loadCompressedFont(Antonio_SemiBold40pt7b_Properties);
    antonioFonts.small = FontLoader::loadCompressedFont(Antonio_SemiBold20pt7b_Properties);

    FONT_BIG = antonioFonts.big;
    FONT_MEDIUM = antonioFonts.medium;
    FONT_SMALL = antonioFonts.small;
  } else if (fontName == FontNames::OSWALD) {
    // Load Oswald fonts
    oswaldFonts.big = FontLoader::loadCompressedFont(Oswald_Medium80pt7b_Properties);
    oswaldFonts.medium = FontLoader::loadCompressedFont(Oswald_Medium30pt7b_Properties);
    oswaldFonts.small = FontLoader::loadCompressedFont(Oswald_Medium20pt7b_Properties);

    FONT_BIG = oswaldFonts.big;
    FONT_MEDIUM = oswaldFonts.medium;
    FONT_SMALL = oswaldFonts.small;
  }

  FONT_SATSYMBOL = FontLoader::loadCompressedFont(Satoshi_Symbol90pt7b_Properties);
}

void setupDisplays() {
  // Load fonts based on preference
  String fontName = preferences.getString("fontName", DEFAULT_FONT_NAME);
  loadFonts(fontName);

  // Initialize displays
  std::lock_guard<std::mutex> lockMcp(mcpMutex);
  for (uint i = 0; i < NUM_SCREENS; i++) {
    displays[i].init(0, true, 30);
  }

  // Create update queue and task
  updateQueue = xQueueCreate(UPDATE_QUEUE_SIZE, sizeof(UpdateDisplayTaskItem));
  xTaskCreate(prepareDisplayUpdateTask, "PrepareUpd", EPD_TASK_STACK_SIZE * 2, NULL, 11, NULL);

  // Create display update tasks
  for (uint i = 0; i < NUM_SCREENS; i++) {
    int *taskParam = new int;
    *taskParam = i;
    xTaskCreate(updateDisplay, ("EpdUpd" + String(i)).c_str(), EPD_TASK_STACK_SIZE, taskParam, 11, &tasks[i]);
  }

  // Check for storage mode (prevents burn-in)
  if (mcp1.read1(0) == LOW) {
    setFgColor(GxEPD_BLACK);
    setBgColor(GxEPD_WHITE);
    epdContent.fill("");
  } else {
    // Initialize with custom text or default
    String customText = preferences.getString("displayText", DEFAULT_BOOT_TEXT);
    std::array<String, NUM_SCREENS> newContent;
    newContent.fill(" ");
    
    for (size_t i = 0; i < std::min(customText.length(), (size_t)NUM_SCREENS); i++) {
      newContent[i] = String(customText[i]);
    }
    
    epdContent = newContent;
  }

  setEpdContent(epdContent);
}

void setEpdContent(std::array<String, NUM_SCREENS> newEpdContent)
{
    setEpdContent(newEpdContent, false);
}

void setEpdContent(std::array<std::string, NUM_SCREENS> newEpdContent)
{
    std::array<String, NUM_SCREENS> conv;

    for (size_t i = 0; i < newEpdContent.size(); ++i)
    {
        conv[i] = String(newEpdContent[i].c_str());
    }

    return setEpdContent(conv);
}

void setEpdContent(std::array<String, NUM_SCREENS> newEpdContent,
                   bool forceUpdate)
{
    std::lock_guard<std::mutex> lock(epdUpdateMutex);

    waitUntilNoneBusy();

    for (uint i = 0; i < NUM_SCREENS; i++)
    {
        if (newEpdContent[i].compareTo(currentEpdContent[i]) != 0 || forceUpdate)
        {
            epdContent[i] = newEpdContent[i];
            UpdateDisplayTaskItem dispUpdate = {i};
            xQueueSend(updateQueue, &dispUpdate, portMAX_DELAY);
        }
    }
}

void prepareDisplayUpdateTask(void *pvParameters)
{
    UpdateDisplayTaskItem receivedItem;

    while (1)
    {
        // Wait for a work item to be available in the queue
        if (xQueueReceive(updateQueue, &receivedItem, portMAX_DELAY))
        {
            uint epdIndex = receivedItem.dispNum;
            std::lock_guard<std::mutex> lock(epdMutex[epdIndex]);
            // displays[epdIndex].init(0, false); // Little longer reset duration
            // because of MCP

            bool updatePartial = true;

            if (epdContent[epdIndex].length() > 1 && strstr(epdContent[epdIndex].c_str(), "/") != NULL)
            {
                String top = epdContent[epdIndex].substring(
                    0, epdContent[epdIndex].indexOf("/"));
                String bottom = epdContent[epdIndex].substring(
                    epdContent[epdIndex].indexOf("/") + 1);
                splitText(epdIndex, top, bottom, updatePartial);
            }
            else if (epdContent[epdIndex].startsWith(F("qr")))
            {
                renderQr(epdIndex, epdContent[epdIndex], updatePartial);
            }
            else if (epdContent[epdIndex].startsWith(F("mdi")))
            {
                bool updated = renderIcon(epdIndex, epdContent[epdIndex], updatePartial);
                if (!updated)
                {
                    continue;
                }
            }
            else if (epdContent[epdIndex].length() > 5)
            {
                renderText(epdIndex, epdContent[epdIndex], updatePartial);
            }
            else
            {
                if (epdContent[epdIndex].length() == 2)
                {
                    showChars(epdIndex, epdContent[epdIndex], updatePartial, FONT_BIG);
                }
                else if (epdContent[epdIndex].length() > 1 && epdContent[epdIndex].indexOf(".") == -1)
                {
                    if (epdContent[epdIndex].equals("STS"))
                    {
                        showDigit(epdIndex, 'S', updatePartial,
                                  FONT_SATSYMBOL);
                    }
                    else
                    {
                        showChars(epdIndex, epdContent[epdIndex], updatePartial,
                                  FONT_MEDIUM);
                    }
                }
                else
                {

                    showDigit(epdIndex, epdContent[epdIndex].c_str()[0], updatePartial,
                              FONT_BIG);
                }
            }

            xTaskNotifyGive(tasks[epdIndex]);
        }
    }
}

extern "C" void updateDisplay(void *pvParameters) noexcept
{
    const int epdIndex = *(int *)pvParameters;
    delete (int *)pvParameters;

    for (;;)
    {
        // Wait for the task notification
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        std::lock_guard<std::mutex> lock(epdMutex[epdIndex]);

        {
            std::lock_guard<std::mutex> lockMcp(mcpMutex);

            displays[epdIndex].init(0, false, 40);
        }
        uint count = 0;
        while (EPD_BUSY[epdIndex].digitalRead() == HIGH || count < 10)
        {
            vTaskDelay(pdMS_TO_TICKS(100));
            count++;
        }

        bool updatePartial = true;

        // Full Refresh every x minutes
        if (!lastFullRefresh[epdIndex] ||
            (millis() - lastFullRefresh[epdIndex]) >
                (preferences.getUInt("fullRefreshMin",
                                     DEFAULT_MINUTES_FULL_REFRESH) *
                 60 * 1000))
        {
            updatePartial = false;
        }

        char tries = 0;
        while (tries < 3)
        {
            if (displays[epdIndex].displayWithReturn(updatePartial))
            {
                displays[epdIndex].powerOff();
                currentEpdContent[epdIndex] = epdContent[epdIndex];
                if (!updatePartial)
                    lastFullRefresh[epdIndex] = millis();

                if (eventSourceTaskHandle != NULL)
                    xTaskNotifyGive(eventSourceTaskHandle);

                break;
            }

            vTaskDelay(pdMS_TO_TICKS(100));
            tries++;
        }
    }
}

void splitText(const uint dispNum, const String &top, const String &bottom,
               bool partial)
{
    if (preferences.getBool("verticalDesc", DEFAULT_VERTICAL_DESC) && dispNum == 0)
    {
        displays[dispNum].setRotation(1);
    }
    else
    {
        displays[dispNum].setRotation(2);
    }
    displays[dispNum].setFont(FONT_SMALL);
    displays[dispNum].setTextColor(getFgColor());

    // Top text
    int16_t ttbx, ttby;
    uint16_t ttbw, ttbh;
    displays[dispNum].getTextBounds(top, 0, 0, &ttbx, &ttby, &ttbw, &ttbh);
    uint16_t tx = ((displays[dispNum].width() - ttbw) / 2) - ttbx;
    uint16_t ty =
        ((displays[dispNum].height() - ttbh) / 2) - ttby - ttbh / 2 - 12;

    // Bottom text
    int16_t tbbx, tbby;
    uint16_t tbbw, tbbh;
    displays[dispNum].getTextBounds(bottom, 0, 0, &tbbx, &tbby, &tbbw, &tbbh);
    uint16_t bx = ((displays[dispNum].width() - tbbw) / 2) - tbbx;
    uint16_t by =
        ((displays[dispNum].height() - tbbh) / 2) - tbby + tbbh / 2 + 12;

    // Make separator as wide as the shortest text.
    uint16_t lineWidth, lineX;
    if (tbbw < ttbh)
        lineWidth = tbbw;
    else
        lineWidth = ttbw;
    lineX = round((displays[dispNum].width() - lineWidth) / 2);

    displays[dispNum].fillScreen(getBgColor());
    displays[dispNum].setCursor(tx, ty);
    displays[dispNum].print(top);
    displays[dispNum].fillRoundRect(lineX, displays[dispNum].height() / 2 - 3,
                                    lineWidth, 6, 3, getFgColor());
    displays[dispNum].setCursor(bx, by);
    displays[dispNum].print(bottom);
}

// Consolidate common display setup code into a helper function
void setupDisplay(const uint dispNum, const GFXfont *font)
{
    displays[dispNum].setRotation(2);
    displays[dispNum].setFont(font);
    displays[dispNum].setTextColor(getFgColor());
    displays[dispNum].fillScreen(getBgColor());
}

void showDigit(const uint dispNum, char chr, bool partial, const GFXfont *font)
{
    String str(chr);
    if (chr == '.')
    {
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

    if (chr == '.')
    {
        displays[dispNum].fillRect(0, 0, displays[dispNum].width(),
                                   round(displays[dispNum].height() * 0.67), getBgColor());
    }
}

int16_t calculateDescent(const GFXfont *font)
{
    int16_t maxDescent = 0;
    for (uint16_t i = font->first; i <= font->last; i++)
    {
        GFXglyph *glyph = &font->glyph[i - font->first];
        int16_t descent = glyph->yOffset;
        if (descent > maxDescent)
        {
            maxDescent = descent;
        }
    }
    return maxDescent;
}

void showChars(const uint dispNum, const String &chars, bool partial,
               const GFXfont *font)
{
    setupDisplay(dispNum, font);

    int16_t tbx, tby;
    uint16_t tbw, tbh;
    displays[dispNum].getTextBounds(chars, 0, 0, &tbx, &tby, &tbw, &tbh);

    // center the bounding box by transposition of the origin:
    uint16_t x = ((displays[dispNum].width() - tbw) / 2) - tbx;
    uint16_t y = ((displays[dispNum].height() - tbh) / 2) - tby;

    for (int i = 0; i < chars.length(); i++)
    {
        char c = chars[i];
        if (c == '.' || c == ',')
        {
            // For the dot, calculate its specific descent
            GFXglyph *dotGlyph = &font->glyph[c - font->first];
            int16_t dotDescent = dotGlyph->yOffset;

            // Draw the dot with adjusted y-position
            displays[dispNum].setCursor(x, y + dotDescent + dotGlyph->height + 8);
            displays[dispNum].print(c);
        }
        else
        {
            // For other characters, use the original y-position
            displays[dispNum].setCursor(x, y);
            displays[dispNum].print(c);
        }

        // Move x-position for the next character
        x += font->glyph[c - font->first].xAdvance;
    }
}

int getBgColor() { return bgColor; }

int getFgColor() { return fgColor; }

void setBgColor(int color) { bgColor = color; }

void setFgColor(int color) { fgColor = color; }

std::array<String, NUM_SCREENS> getCurrentEpdContent()
{
    return currentEpdContent;
}
void renderText(const uint dispNum, const String &text, bool partial)
{
    displays[dispNum].setRotation(2);
    displays[dispNum].setPartialWindow(0, 0, displays[dispNum].width(),
                                       displays[dispNum].height());
    displays[dispNum].fillScreen(GxEPD_WHITE);
    displays[dispNum].setTextColor(GxEPD_BLACK);
    displays[dispNum].setCursor(0, 50);

    std::stringstream ss;
    ss.str(text.c_str());

    std::string line;

    while (std::getline(ss, line, '\n'))
    {
        if (line.rfind("*", 0) == 0)
        {
            line.erase(std::remove(line.begin(), line.end(), '*'), line.end());

            displays[dispNum].setFont(&FreeSansBold9pt7b);
            displays[dispNum].println(line.c_str());
        }
        else
        {
            displays[dispNum].setFont(&FreeSans9pt7b);
            displays[dispNum].println(line.c_str());
        }
    }
}

bool renderIcon(const uint dispNum, const String &text, bool partial)
{
    displays[dispNum].setRotation(2);

    displays[dispNum].setPartialWindow(0, 0, displays[dispNum].width(),
                                       displays[dispNum].height());
    displays[dispNum].fillScreen(getBgColor());
    displays[dispNum].setTextColor(getFgColor());

    uint iconIndex = 0;
    uint width = 122;
    uint height = 122;
    if (text.endsWith("rocket"))
    {
        iconIndex = 1;
    }
    else if (text.endsWith("lnbolt"))
    {
        iconIndex = 2;
    }
    else if (text.endsWith("bitaxe"))
    {
        width = 88;
        height = 220;
        iconIndex = 3;
    }
    else if (text.endsWith("miningpool"))
    {
        LogoData logo = MiningPoolStatsFetch::getInstance().getLogo();

        if (logo.size == 0)
        {
            Serial.println(F("No logo found"));
            return false;
        }

        int x_offset = (displays[dispNum].width() - logo.width) / 2;
        int y_offset = (displays[dispNum].height() - logo.height) / 2;
        // Close the file

        displays[dispNum].drawInvertedBitmap(x_offset, y_offset, logo.data, logo.width, logo.height, getFgColor());
        return true;
    }

    int x_offset = (displays[dispNum].width() - width) / 2;
    int y_offset = (displays[dispNum].height() - height) / 2;

    displays[dispNum].drawInvertedBitmap(x_offset, y_offset, epd_icons_allArray[iconIndex], width, height, getFgColor());

    return true;
    //  displays[dispNum].drawInvertedBitmap(0,0, getOceanIcon(), 122, 250, getFgColor());
}

void renderQr(const uint dispNum, const String &text, bool partial)
{
#ifdef USE_QR

    uint8_t tempBuffer[800];
    bool ok = qrcodegen_encodeText(
        text.substring(2).c_str(), tempBuffer, qrcode, qrcodegen_Ecc_LOW,
        qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, true);

    const int size = qrcodegen_getSize(qrcode);

    const int padding = floor(float(displays[dispNum].width() - (size * 4)) / 2);
    const int paddingY =
        floor(float(displays[dispNum].height() - (size * 4)) / 2);
    displays[dispNum].setRotation(2);

    displays[dispNum].setPartialWindow(0, 0, displays[dispNum].width(),
                                       displays[dispNum].height());
    displays[dispNum].fillScreen(GxEPD_WHITE);
    const int border = 0;

    for (int y = -border; y < size * 4 + border; y++)
    {
        for (int x = -border; x < size * 4 + border; x++)
        {
            displays[dispNum].drawPixel(
                padding + x, paddingY + y,
                qrcodegen_getModule(qrcode, floor(float(x) / 4), floor(float(y) / 4))
                    ? GxEPD_BLACK
                    : GxEPD_WHITE);
        }
    }
#endif
}

void waitUntilNoneBusy()
{
    for (int i = 0; i < NUM_SCREENS; i++)
    {
        uint count = 0;
        while (EPD_BUSY[i].digitalRead())
        {
            count++;
            vTaskDelay(BUSY_RETRY_DELAY);

            if (count == BUSY_TIMEOUT_COUNT)
            {
                vTaskDelay(pdMS_TO_TICKS(100));
            }
            else if (count > BUSY_TIMEOUT_COUNT + 5)
            {
                log_e("Display %d busy timeout", i);
                break;
            }
        }
    }
}