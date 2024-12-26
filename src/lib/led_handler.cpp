#include "led_handler.hpp"

TaskHandle_t ledTaskHandle = NULL;
QueueHandle_t ledTaskQueue = NULL;
Adafruit_NeoPixel pixels(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
uint ledTaskParams;

#ifdef HAS_FRONTLIGHT
constexpr uint16_t FL_FADE_STEP = 25;

bool frontlightOn = false;
bool flInTransition = false;

void frontlightFlash(int flDelayTime)
{
  if (preferences.getBool("flDisable"))
    return;

  if (frontlightOn)
  {
    frontlightFadeOutAll(flDelayTime, true);
    frontlightFadeInAll(flDelayTime, true);
  }
  else
  {
    frontlightFadeInAll(flDelayTime, true);
    frontlightFadeOutAll(flDelayTime, true);
  }
}

void frontlightFadeInAll()
{
  frontlightFadeInAll(preferences.getUInt("flEffectDelay"));
}

void frontlightFadeOutAll()
{
  frontlightFadeOutAll(preferences.getUInt("flEffectDelay"));
}

void frontlightFadeIn(uint num)
{
  frontlightFadeIn(num, preferences.getUInt("flEffectDelay"));
}

void frontlightFadeOut(uint num)
{
  frontlightFadeOut(num, preferences.getUInt("flEffectDelay"));
}

void frontlightSetBrightness(uint brightness)
{
  if (brightness > 4096)
  {
    return;
  }

  for (int ledPin = 0; ledPin <= NUM_SCREENS; ledPin++)
  {
    flArray.setPWM(ledPin, 0, brightness);
  }
}

void frontlightFadeInAll(int flDelayTime)
{
  frontlightFadeInAll(flDelayTime, false);
}

void frontlightFadeInAll(int flDelayTime, bool staggered)
{
  if (preferences.getBool("flDisable") || frontlightIsOn() || flInTransition)
    return;

  flInTransition = true;

  const int maxBrightness = preferences.getUInt("flMaxBrightness");

  if (staggered)
  {
    int step = FL_FADE_STEP;
    int staggerDelay = flDelayTime / NUM_SCREENS;

    for (int dutyCycle = 0; dutyCycle <= maxBrightness + (NUM_SCREENS - 1) * maxBrightness / NUM_SCREENS; dutyCycle += step)
    {
      for (int ledPin = 0; ledPin < NUM_SCREENS; ledPin++)
      {
        int ledBrightness = dutyCycle - ledPin * maxBrightness / NUM_SCREENS;
        if (ledBrightness < 0)
          ledBrightness = 0;
        else if (ledBrightness > maxBrightness)
          ledBrightness = maxBrightness;

        flArray.setPWM(ledPin + 1, 0, ledBrightness);
      }
      vTaskDelay(pdMS_TO_TICKS(staggerDelay));
    }
  }
  else
  {
    for (int dutyCycle = 0; dutyCycle <= maxBrightness; dutyCycle += FL_FADE_STEP)
    {
      for (int ledPin = 0; ledPin <= NUM_SCREENS; ledPin++)
      {
        flArray.setPWM(ledPin, 0, dutyCycle);
      }
      vTaskDelay(pdMS_TO_TICKS(flDelayTime));
    }
  }
  frontlightOn = true;
  flInTransition = false;
}

void frontlightFadeOutAll(int flDelayTime)
{
  frontlightFadeOutAll(flDelayTime, false);
}

void frontlightFadeOutAll(int flDelayTime, bool staggered)
{
  if (preferences.getBool("flDisable"))
    return;
  if (!frontlightIsOn())
    return;
  if (flInTransition)
    return;
  flInTransition = true;

  if (staggered)
  {
    int maxBrightness = preferences.getUInt("flMaxBrightness");
    int step = FL_FADE_STEP;
    int staggerDelay = flDelayTime / NUM_SCREENS;

    for (int dutyCycle = maxBrightness; dutyCycle >= 0; dutyCycle -= step)
    {
      for (int ledPin = 0; ledPin < NUM_SCREENS; ledPin++)
      {
        int ledBrightness = dutyCycle - (NUM_SCREENS - 1 - ledPin) * maxBrightness / NUM_SCREENS;
        if (ledBrightness < 0)
          ledBrightness = 0;
        else if (ledBrightness > maxBrightness)
          ledBrightness = maxBrightness;

        flArray.setPWM(ledPin + 1, 0, ledBrightness);
      }
      vTaskDelay(pdMS_TO_TICKS(staggerDelay));
    }
  }
  else
  {
    for (int dutyCycle = preferences.getUInt("flMaxBrightness"); dutyCycle >= 0; dutyCycle -= FL_FADE_STEP)
    {
      for (int ledPin = 0; ledPin <= NUM_SCREENS; ledPin++)
      {
        flArray.setPWM(ledPin, 0, dutyCycle);
      }
      vTaskDelay(pdMS_TO_TICKS(flDelayTime));
    }
  }

  flArray.allOFF();
  frontlightOn = false;
  flInTransition = false;
}

std::vector<uint16_t> frontlightGetStatus()
{
  std::vector<uint16_t> statuses;
  for (int ledPin = 1; ledPin <= NUM_SCREENS; ledPin++)
  {
    uint16_t a = 0, b = 0;
    flArray.getPWM(ledPin, &a, &b);
    statuses.push_back(round(b - a / 4096));
  }

  return statuses;
}

bool frontlightIsOn()
{
  return frontlightOn;
}

void frontlightFadeIn(uint num, int flDelayTime)
{
  if (preferences.getBool("flDisable"))
    return;
  for (int dutyCycle = 0; dutyCycle <= preferences.getUInt("flMaxBrightness"); dutyCycle += 5)
  {
    flArray.setPWM(num, 0, dutyCycle);
    vTaskDelay(pdMS_TO_TICKS(flDelayTime));
  }
}

void frontlightFadeOut(uint num, int flDelayTime)
{
  if (preferences.getBool("flDisable"))
    return;
  if (!frontlightIsOn())
    return;

  for (int dutyCycle = preferences.getUInt("flMaxBrightness"); dutyCycle >= 0; dutyCycle -= 5)
  {
    flArray.setPWM(num, 0, dutyCycle);
    vTaskDelay(pdMS_TO_TICKS(flDelayTime));
  }
}
#endif

void ledTask(void *parameter)
{
  while (1)
  {
    if (ledTaskQueue != NULL)
    {
      if (xQueueReceive(ledTaskQueue, &ledTaskParams, portMAX_DELAY) ==
          pdPASS)
      {

        if (preferences.getBool("disableLeds", DEFAULT_DISABLE_LEDS))
        {
          continue;
        }

        std::array<uint32_t, NEOPIXEL_COUNT> oldLights;

        // get current state
        for (int i = 0; i < NEOPIXEL_COUNT; i++)
        {
          oldLights[i] = pixels.getPixelColor(i);
        }
#ifdef HAS_FRONTLIGHT
        uint flDelayTime = preferences.getUInt("flEffectDelay");
#endif
        switch (ledTaskParams)
        {
        case LED_POWER_TEST:
#ifdef HAS_FRONTLIGHT
          frontlightFadeInAll(preferences.getUInt("flEffectDelay"), true);
#endif
          ledRainbow(20);
          pixels.clear();
          break;
        case LED_EFFECT_WIFI_CONNECT_ERROR:
          blinkDelayTwoColor(100, 3, pixels.Color(8, 161, 236),
                             pixels.Color(255, 0, 0));
          break;
        case LED_FLASH_ERROR:
          blinkDelayColor(250, 3, 255, 0, 0);
          break;
        case LED_EFFECT_HEARTBEAT:
          blinkDelayColor(150, 2, 0, 0, 255);
          break;
        case LED_DATA_BLOCK_ERROR:
          blinkDelayColor(150, 2, 128, 0, 128);
          break;
        case LED_DATA_PRICE_ERROR:
          blinkDelayColor(150, 2, 177, 90, 31);
          break;
        case LED_FLASH_IDENTIFY:
          blinkDelayTwoColor(100, 2, pixels.Color(255, 0, 0),
                             pixels.Color(0, 255, 255));
          blinkDelayTwoColor(100, 2, pixels.Color(0, 255, 0),
                             pixels.Color(0, 0, 255));
          break;
        case LED_EFFECT_WIFI_CONNECT_SUCCESS:
        case LED_FLASH_SUCCESS:
          blinkDelayColor(150, 3, 0, 255, 0);
          break;
        case LED_PROGRESS_100:
          pixels.setPixelColor(0, pixels.Color(0, 255, 0));
        case LED_PROGRESS_75:
          pixels.setPixelColor(1, pixels.Color(0, 255, 0));
        case LED_PROGRESS_50:
          pixels.setPixelColor(2, pixels.Color(0, 255, 0));
        case LED_PROGRESS_25:
          pixels.setPixelColor(3, pixels.Color(0, 255, 0));
          pixels.show();
          break;
        case LED_EFFECT_NOSTR_ZAP:
        {
#ifdef HAS_FRONTLIGHT
          bool frontlightWasOn = false;

          if (preferences.getBool("flFlashOnZap", DEFAULT_FL_FLASH_ON_ZAP))
          {
            if (frontlightOn)
            {
              frontlightWasOn = true;
              frontlightFadeOutAll(flDelayTime, true);
            }
            else
            {
              frontlightFadeInAll(flDelayTime, true);
            }
          }
#endif
          for (int flash = 0; flash < random(7, 10); flash++)
          {
            lightningStrike();
            delay(random(50, 150));
          }
          // blinkDelayColor(250, 3, 142, 48, 235);
          //   blinkDelayTwoColor(250, 3, pixels.Color(142, 48, 235),
          //                    pixels.Color(169, 21, 255));
#ifdef HAS_FRONTLIGHT
          if (preferences.getBool("flFlashOnZap", DEFAULT_FL_FLASH_ON_ZAP))
          {
            vTaskDelay(pdMS_TO_TICKS(10));
            if (frontlightWasOn)
            {
              frontlightFadeInAll(flDelayTime, true);
            }
            else
            {
              frontlightFadeOutAll(flDelayTime, true);
            }
          }
#endif
          break;
        }
        case LED_FLASH_UPDATE:
          blinkDelayTwoColor(250, 3, pixels.Color(0, 230, 0),
                             pixels.Color(230, 230, 0));
          break;
        case LED_FLASH_BLOCK_NOTIFY:
        {
#ifdef HAS_FRONTLIGHT
          bool frontlightWasOn = false;

          if (preferences.getBool("flFlashOnUpd", DEFAULT_FL_FLASH_ON_UPDATE))
          {
            if (frontlightOn)
            {
              frontlightWasOn = true;
              frontlightFadeOutAll(flDelayTime, true);
            }
            else
            {
              frontlightFadeInAll(flDelayTime, true);
            }
          }
#endif
          blinkDelayTwoColor(250, 3, pixels.Color(224, 67, 0),
                             pixels.Color(8, 2, 0));
#ifdef HAS_FRONTLIGHT
          if (preferences.getBool("flFlashOnUpd", DEFAULT_FL_FLASH_ON_UPDATE))
          {
            vTaskDelay(pdMS_TO_TICKS(10));
            if (frontlightWasOn)
            {
              frontlightFadeInAll(flDelayTime, true);
            }
            else
            {
              frontlightFadeOutAll(flDelayTime, true);
            }
          }
#endif
          break;
        }
        case LED_EFFECT_WIFI_WAIT_FOR_CONFIG:
          blinkDelayTwoColor(100, 1, pixels.Color(8, 161, 236),
                             pixels.Color(156, 225, 240));
          break;
        case LED_EFFECT_WIFI_ERASE_SETTINGS:
          blinkDelay(100, 3);
          break;
        case LED_EFFECT_WIFI_CONNECTING:
          for (int i = NEOPIXEL_COUNT; i >= 0; i--)
          {
            for (int j = NEOPIXEL_COUNT; j >= 0; j--)
            {
              if (j == i)
              {
                pixels.setPixelColor(i, pixels.Color(16, 197, 236));
              }
              else
              {
                pixels.setPixelColor(j, pixels.Color(0, 0, 0));
              }
            }
            pixels.show();
            vTaskDelay(pdMS_TO_TICKS(100));
          }
          break;
        case LED_EFFECT_PAUSE_TIMER:
          for (int i = NEOPIXEL_COUNT; i >= 0; i--)
          {
            for (int j = NEOPIXEL_COUNT; j >= 0; j--)
            {
              uint32_t c = pixels.Color(0, 0, 0);
              if (i == j)
                c = pixels.Color(0, 255, 0);
              pixels.setPixelColor(j, c);
            }

            pixels.show();

            delay(100);
          }
          pixels.setPixelColor(0, pixels.Color(255, 0, 0));
          pixels.show();

          delay(900);

          pixels.clear();
          pixels.show();
          break;
        case LED_EFFECT_START_TIMER:
          pixels.clear();
          pixels.setPixelColor((NEOPIXEL_COUNT - 1), pixels.Color(255, 0, 0));
          pixels.show();

          delay(900);

          for (int i = NEOPIXEL_COUNT; i--; i > 0)
          {
            for (int j = NEOPIXEL_COUNT; j--; j > 0)
            {
              uint32_t c = pixels.Color(0, 0, 0);
              if (i == j)
                c = pixels.Color(0, 255, 0);

              pixels.setPixelColor(j, c);
            }

            pixels.show();

            delay(100);
          }

          pixels.clear();
          pixels.show();
          break;
        }

        // revert to previous state unless power test

        for (int i = 0; i < NEOPIXEL_COUNT; i++)
        {
          pixels.setPixelColor(i, oldLights[i]);
        }

        pixels.show();
      }
    }
  }
}

void setupLeds()
{
  pixels.begin();
  pixels.setBrightness(preferences.getUInt("ledBrightness", DEFAULT_LED_BRIGHTNESS));
  pixels.clear();
  pixels.show();
  setupLedTask();
  if (preferences.getBool("ledTestOnPower", DEFAULT_LED_TEST_ON_POWER))
  {
    while (!ledTaskQueue)
    {
      delay(1);
      // wait until queue is available
    }
    queueLedEffect(LED_POWER_TEST);
  }
}

void setupLedTask()
{
  ledTaskQueue = xQueueCreate(5, sizeof(uint));

  xTaskCreate(ledTask, "LedTask", 2048, NULL, 10, &ledTaskHandle);
}

void blinkDelay(int d, int times)
{
  for (int j = 0; j < times; j++)
  {
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

void blinkDelayColor(int d, int times, uint r, uint g, uint b) 
{
  for (int j = 0; j < times; j++)
  {
    for (int i = 0; i < NEOPIXEL_COUNT; i++)
    {
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

void blinkDelayTwoColor(int d, int times, const uint32_t& c1, const uint32_t& c2)
{
  for (int j = 0; j < times; j++)
  {
    for (int i = 0; i < NEOPIXEL_COUNT; i++)
    {
      pixels.setPixelColor(i, c1);
    }
    pixels.show();
    vTaskDelay(pdMS_TO_TICKS(d));

    for (int i = 0; i < NEOPIXEL_COUNT; i++)
    {
      pixels.setPixelColor(i, c2);
    }
    pixels.show();
    vTaskDelay(pdMS_TO_TICKS(d));
  }
  pixels.clear();
  pixels.show();
}

void clearLeds()
{
  preferences.putBool("ledStatus", false);
  pixels.clear();
  pixels.show();
}

void setLights(int r, int g, int b) { setLights(pixels.Color(r, g, b)); }

void setLights(uint32_t color)
{
  bool ledStatus = true;

  for (int i = 0; i < NEOPIXEL_COUNT; i++)
  {
    pixels.setPixelColor(i, color);
  }
  pixels.show();

  if (color == pixels.Color(0, 0, 0))
  {
    ledStatus = false;
  }
  else
  {
    saveLedState();
  }
  preferences.putBool("ledStatus", ledStatus);
}

void saveLedState()
{
  for (int i = 0; i < pixels.numPixels(); i++)
  {
    int pixelColor = pixels.getPixelColor(i);
    char key[12];
    snprintf(key, 12, "%s%d", "ledColor_", i);
    preferences.putUInt(key, pixelColor);
  }

  xTaskNotifyGive(eventSourceTaskHandle);
}

void restoreLedState()
{
  for (int i = 0; i < pixels.numPixels(); i++)
  {
    char key[12];
    snprintf(key, 12, "%s%d", "ledColor_", i);
    uint pixelColor = preferences.getUInt(key, pixels.Color(0, 0, 0));
    pixels.setPixelColor(i, pixelColor);
  }

  pixels.show();
}

QueueHandle_t getLedTaskQueue() { return ledTaskQueue; }

bool queueLedEffect(uint effect)
{
  if (ledTaskQueue == NULL)
  {
    return false;
  }

  uint flashType = effect;
  xQueueSend(ledTaskQueue, &flashType, portMAX_DELAY);
}

void ledRainbow(int wait)
{
  // Hue of first pixel runs 5 complete loops through the color wheel.
  // Color wheel has a range of 65536 but it's OK if we roll over, so
  // just count from 0 to 5*65536. Adding 256 to firstPixelHue each time
  // means we'll make 5*65536/256 = 1280 passes through this loop:
  for (long firstPixelHue = 0; firstPixelHue < 5 * 65536;
       firstPixelHue += 256)
  {
    // strip.rainbow() can take a single argument (first pixel hue) or
    // optionally a few extras: number of rainbow repetitions (default 1),
    // saturation and value (brightness) (both 0-255, similar to the
    // ColorHSV() function, default 255), and a true/false flag for whether
    // to apply gamma correction to provide 'truer' colors (default true).
    pixels.rainbow(firstPixelHue);
    // Above line is equivalent to:
    // strip.rainbow(firstPixelHue, 1, 255, 255, true);
    pixels.show(); // Update strip with new contents
    delayMicroseconds(wait);
    //        vTaskDelay(pdMS_TO_TICKS(wait)); // Pause for a moment
  }
}

void ledTheaterChase(uint32_t color, int wait)
{
  for (int a = 0; a < 10; a++)
  { // Repeat 10 times...
    for (int b = 0; b < 3; b++)
    {                 //  'b' counts from 0 to 2...
      pixels.clear(); //   Set all pixels in RAM to 0 (off)
      // 'c' counts up from 'b' to end of strip in steps of 3...
      for (int c = b; c < pixels.numPixels(); c += 3)
      {
        pixels.setPixelColor(c, color); // Set pixel 'c' to value 'color'
      }
      pixels.show();                   // Update strip with new contents
      vTaskDelay(pdMS_TO_TICKS(wait)); // Pause for a moment
    }
  }
}

void ledTheaterChaseRainbow(int wait)
{
  int firstPixelHue = 0; // First pixel starts at red (hue 0)
  for (int a = 0; a < 30; a++)
  { // Repeat 30 times...
    for (int b = 0; b < 3; b++)
    {                 //  'b' counts from 0 to 2...
      pixels.clear(); //   Set all pixels in RAM to 0 (off)
      // 'c' counts up from 'b' to end of strip in increments of 3...
      for (int c = b; c < pixels.numPixels(); c += 3)
      {
        // hue of pixel 'c' is offset by an amount to make one full
        // revolution of the color wheel (range 65536) along the length
        // of the strip (strip.numPixels() steps):
        int hue = firstPixelHue + c * 65536L / pixels.numPixels();
        uint32_t color = pixels.gamma32(pixels.ColorHSV(hue)); // hue -> RGB
        pixels.setPixelColor(c, color);                        // Set pixel 'c' to value 'color'
      }
      pixels.show();                   // Update strip with new contents
      vTaskDelay(pdMS_TO_TICKS(wait)); // Pause for a moment
      firstPixelHue += 65536 / 90;     // One cycle of color wheel over 90 frames
    }
  }
}

void lightningStrike()
{
  uint32_t PURPLE = pixels.Color(128, 0, 128);
  uint32_t YELLOW = pixels.Color(255, 226, 41);

  // Randomly choose which LEDs to light up
  for (int i = 0; i < pixels.numPixels(); i++)
  {
    if (random(2) == 0)
    { // 50% chance for each LED
      pixels.setPixelColor(i, YELLOW);
    }
    else
    {
      pixels.setPixelColor(i, PURPLE);
    }
  }
  pixels.show();

  delay(random(10, 50)); // Flash duration

  // Return to purple background
  // setAllPixels(PURPLE);
}

Adafruit_NeoPixel getPixels() { return pixels; }
