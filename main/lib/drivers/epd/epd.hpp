#pragma once

#include "lib/util/gzip_decompressor.hpp"
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <GxEPD2_BW.h>

#include <array>
#include <mcp23x17_pin.hpp>
#include <memory>
#include <mutex>
#include <native_pin.hpp>
#include <regex>

#include "fonts/fonts.hpp"
#include "icons/icons.h"
#include "lib/data_sources/mining_pool_stats_fetch.hpp"
#include "lib/system/config.hpp"
#include "lib/system/shared.hpp"

// Font includes
#include "fonts/antonio-semibold20.h"
#include "fonts/antonio-semibold40.h"
#include "fonts/antonio-semibold90.h"

// Oswald fonts
#include "fonts/oswald-medium20.h"
#include "fonts/oswald-medium30.h"
#include "fonts/oswald-medium80.h"

#include "fonts/sats-symbol.h"

#ifdef USE_QR
#include "qrcodegen.h"
#endif

struct UpdateDisplayTaskItem {
  char dispNum;
};

struct FontFamily {
  GFXfont *big;
  GFXfont *medium;
  GFXfont *small;
};

class EPDManager {
public:
  static EPDManager &getInstance();

  // Delete copy constructor and assignment operator
  EPDManager(const EPDManager &) = delete;
  EPDManager &operator=(const EPDManager &) = delete;

  void initialize();
  void forceFullRefresh();
  void loadFonts(const String &fontName);
  void setContent(const std::array<String, NUM_SCREENS> &newContent,
                  bool forceUpdate = false);
  void setContent(const std::array<std::string, NUM_SCREENS> &newContent);
  std::array<String, NUM_SCREENS> getCurrentContent() const;

  int getBackgroundColor() const { return bgColor; }
  int getForegroundColor() const { return fgColor; }
  void setBackgroundColor(int color) { bgColor = color; }
  void setForegroundColor(int color) { fgColor = color; }
  void waitUntilNoneBusy();

private:
  EPDManager();  // Private constructor for singleton
  ~EPDManager(); // Private destructor

  // The bool partial parameter that used to decorate every render/show
  // function was never actually consumed inside these bodies; the partial
  // vs full refresh decision happens in updateDisplayTask() via
  // displayWithReturn(updatePartial). Dropping the parameter removes one
  // source of "what does passing true here mean?" confusion.
  void setupDisplay(uint dispNum, const GFXfont *font);
  void splitText(uint dispNum, const String &top, const String &bottom);
  void showDigit(uint dispNum, char chr, const GFXfont *font);
  void showChars(uint dispNum, const String &chars, const GFXfont *font);
  bool renderIcon(uint dispNum, const String &text);
  void renderText(uint dispNum, const String &text);
  void renderQr(uint dispNum, const String &text);

  static void updateDisplayTask(void *pvParameters) noexcept;
  static void prepareDisplayUpdateTask(void *pvParameters);

  // Member variables
  std::array<String, NUM_SCREENS> currentContent;
  std::array<String, NUM_SCREENS> content;
  std::array<uint32_t, NUM_SCREENS> lastFullRefresh;
  std::array<TaskHandle_t, NUM_SCREENS> tasks;
  QueueHandle_t updateQueue;

  FontFamily antonioFonts;
  FontFamily oswaldFonts;
  const GFXfont *fontSmall;
  const GFXfont *fontBig;
  const GFXfont *fontMedium;
  const GFXfont *fontSatsymbol;

  int bgColor;
  int fgColor;

  std::mutex updateMutex;
  std::array<std::mutex, NUM_SCREENS> displayMutexes;

// Pin configurations based on board version
#ifdef IS_BTCLOCK_REV_B
  static Native_Pin EPD_DC;
  static std::array<Native_Pin, NUM_SCREENS> EPD_CS;
  static std::array<Native_Pin, NUM_SCREENS> EPD_BUSY;
  static std::array<MCP23X17_Pin, NUM_SCREENS> EPD_RESET;
#elif defined(IS_BTCLOCK_V8)
  static Native_Pin EPD_DC;
  static std::array<MCP23X17_Pin, NUM_SCREENS> EPD_BUSY;
  static std::array<MCP23X17_Pin, NUM_SCREENS> EPD_CS;
  static std::array<MCP23X17_Pin, NUM_SCREENS> EPD_RESET;
#else
  static Native_Pin EPD_DC;
  static std::array<Native_Pin, NUM_SCREENS> EPD_CS;
  static std::array<Native_Pin, NUM_SCREENS> EPD_BUSY;
  static std::array<MCP23X17_Pin, NUM_SCREENS> EPD_RESET;
#endif

  // Display array
  std::array<GxEPD2_BW<EPD_CLASS, EPD_CLASS::HEIGHT>, NUM_SCREENS> displays;

  static constexpr size_t UPDATE_QUEUE_SIZE = 14;
  static constexpr uint32_t BUSY_TIMEOUT_COUNT = 200;
  static constexpr TickType_t BUSY_RETRY_DELAY = pdMS_TO_TICKS(10);
  static constexpr size_t EPD_TASK_STACK_SIZE =
#ifdef IS_BTCLOCK_V8
      4096
#else
      2048
#endif
      ;
};