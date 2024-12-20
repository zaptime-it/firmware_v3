#pragma once

#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <GxEPD2_BW.h>


#include <mcp23x17_pin.hpp>
#include <mutex>
#include <native_pin.hpp>
#include <regex>

#include "fonts/fonts.hpp"
#include "lib/config.hpp"
#include "lib/shared.hpp"
#include "icons/icons.h"
#include "mining_pool_stats_fetch.hpp"

#ifdef USE_QR
#include "qrcodegen.h"
#endif
// extern TaskHandle_t epdTaskHandle;

typedef struct {
  char dispNum;
} UpdateDisplayTaskItem;

void forceFullRefresh();
void refreshFromMemory();
void setupDisplays();

void splitText(const uint dispNum, const String &top, const String &bottom,
               bool partial);

void showDigit(const uint dispNum, char chr, bool partial, const GFXfont *font);
void showChars(const uint dispNum, const String &chars, bool partial,
               const GFXfont *font);

extern "C" void updateDisplay(void *pvParameters) noexcept;
void updateDisplayAlt(int epdIndex);
void prepareDisplayUpdateTask(void *pvParameters);

int getBgColor();
int getFgColor();
void setBgColor(int color);
void setFgColor(int color);

bool renderIcon(const uint dispNum, const String &text, bool partial);
void renderText(const uint dispNum, const String &text, bool partial);
void renderQr(const uint dispNum, const String &text, bool partial);

void setEpdContent(std::array<String, NUM_SCREENS> newEpdContent,
                   bool forceUpdate);
void setEpdContent(std::array<String, NUM_SCREENS> newEpdContent);

void setEpdContent(std::array<std::string, NUM_SCREENS> newEpdContent);

std::array<String, NUM_SCREENS> getCurrentEpdContent();
void waitUntilNoneBusy();