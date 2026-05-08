#include "ota.hpp"
#include "lib/drivers/leds/led_handler.hpp"
#include "lib/system/pref_keys.hpp"

#include <algorithm>

TaskHandle_t taskOtaHandle = NULL;
bool isOtaUpdating = false;
QueueHandle_t otaQueue;

void setupOTA() {
  if (preferences.getBool("otaEnabled", DEFAULT_OTA_ENABLED)) {
    ArduinoOTA.onStart(onOTAStart);

    ArduinoOTA.onProgress(onOTAProgress);
    ArduinoOTA.onError(onOTAError);
    ArduinoOTA.onEnd(onOTAComplete);

    ArduinoOTA.setHostname(getMyHostname().c_str());
    ArduinoOTA.setMdnsEnabled(false);
    ArduinoOTA.setRebootOnSuccess(false);
    // Require a password for espota/mDNS-based OTA pushes when one is set.
    // Without this, anyone on the LAN could push arbitrary firmware at the
    // device once otaEnabled is true.
    String otaPass = preferences.getString(PrefKeys::OtaPass, "");
    if (otaPass.length() > 0) {
      ArduinoOTA.setPassword(otaPass.c_str());
    }
    ArduinoOTA.begin();
    // downloadUpdate();
    otaQueue = xQueueCreate(1, sizeof(UpdateMessage));

    xTaskCreate(handleOTATask, "handleOTA", 8192, NULL, 20, &taskOtaHandle);
  }
}

void onOTAProgress(unsigned int progress, unsigned int total) {
  // Guard against div-by-zero: if total is < 100 the previous expression
  // `total / 100` evaluated to 0 and triggered a panic.
  uint percentage = 0;
  if (total >= 100) {
    percentage = progress / (total / 100);
  } else if (total > 0) {
    percentage = (progress * 100) / total;
  }
  auto &ledHandler = getLedHandler();
  auto &pixels = ledHandler.getPixels();

  pixels.fill(pixels.Color(0, 255, 0));
  if (percentage < 100) {
    pixels.setPixelColor(0, pixels.Color(0, 0, 0));
  }
  if (percentage < 75) {
    pixels.setPixelColor(1, pixels.Color(0, 0, 0));
  }
  if (percentage < 50) {
    pixels.setPixelColor(2, pixels.Color(0, 0, 0));
  }
  if (percentage < 25) {
    pixels.setPixelColor(3, pixels.Color(0, 0, 0));
  }
  pixels.show();
}

void onOTAStart() {
  EPDManager::getInstance().forceFullRefresh();
  std::array<String, NUM_SCREENS> epdContent = {"U", "P", "D", "A",
                                                "T", "E", "!"};
  EPDManager::getInstance().setContent(epdContent);
  // Stop all timers
  esp_timer_stop(screenRotateTimer);
  esp_timer_stop(minuteTimer);
  isOtaUpdating = true;
  // Stop or suspend all tasks
  //  vTaskSuspend(priceUpdateTaskHandle);
  //    vTaskSuspend(blockUpdateTaskHandle);
  vTaskSuspend(taskScreenRotateTaskHandle);
  vTaskSuspend(workerTaskHandle);
  vTaskSuspend(eventSourceTaskHandle);
  ButtonHandler::suspendTask();

  // stopWebServer();
  auto &blockNotify = BlockNotify::getInstance();
  blockNotify.stop();
}

void handleOTATask(void *parameter) {
  UpdateMessage msg;

  for (;;) {
    if (xQueueReceive(otaQueue, &msg, 0) == pdTRUE) {
      if (msg.updateType == UPDATE_ALL) {
        isOtaUpdating = true;
        getLedHandler().queueEffect(LED_FLASH_UPDATE);
        int resultWebUi = downloadUpdateHandler(UPDATE_WEBUI);
        getLedHandler().queueEffect(LED_FLASH_UPDATE);
        int resultFw = downloadUpdateHandler(UPDATE_FIRMWARE);

        if (resultWebUi == 0 && resultFw == 0) {
          ESP.restart();
        } else {
          getLedHandler().queueEffect(LED_FLASH_ERROR);
          vTaskDelay(pdMS_TO_TICKS(3000));
          ESP.restart();
        }
      }
    }

    ArduinoOTA.handle(); // Allow OTA updates to occur
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

ReleaseInfo getLatestRelease(const String &fileToDownload) {
  String releaseUrl = preferences.getString("gitReleaseUrl");
  ReleaseInfo info = {"", ""};

  auto http = HttpHelper::beginScoped(releaseUrl);
  if (!http)
    return info;

  int httpCode = http->GET();
  if (httpCode != HTTP_CODE_OK)
    return info;

  JsonDocument doc;
  if (deserializeJson(doc, http->getString()) != DeserializationError::Ok)
    return info;

  for (JsonObject asset : doc["assets"].as<JsonArray>()) {
    String assetName = asset["name"].as<String>();
    if (assetName == fileToDownload) {
      info.fileUrl = asset["browser_download_url"].as<String>();
    } else if (assetName == fileToDownload + ".sha256") {
      info.checksumUrl = asset["browser_download_url"].as<String>();
    }
    if (!info.fileUrl.isEmpty() && !info.checksumUrl.isEmpty())
      break;
  }

  return info;
}

int downloadUpdateHandler(char updateType) {
  ReleaseInfo latestRelease;

  switch (updateType) {
  case UPDATE_FIRMWARE:
    latestRelease = getLatestRelease(getFirmwareFilename());
    break;
  case UPDATE_WEBUI:
    latestRelease = getLatestRelease(getWebUiFilename());
    break;
  }

  if (latestRelease.fileUrl.isEmpty() || latestRelease.checksumUrl.isEmpty()) {
    return 503;
  }

  String expectedSHA256 = downloadSHA256(latestRelease.checksumUrl);
  if (expectedSHA256.isEmpty()) {
    return 503;
  }
  expectedSHA256.toLowerCase();

  auto http = HttpHelper::beginScoped(latestRelease.fileUrl);
  if (!http)
    return 503;
  http->setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  int httpCode = http->GET();
  if (httpCode != HTTP_CODE_OK)
    return 503;

  int contentLength = http->getSize();
  if (contentLength <= 0)
    return 503;

  // Stream the payload through both mbedtls (for SHA256) and Update.write()
  // at the same time. The previous implementation malloc'd the entire
  // firmware into RAM first (up to ~1.5 MB), calculated the hash, then
  // handed the buffer to Update. That doubled the peak heap use and
  // reliably OOM'd on 4 MB parts. Since Update validates on Update.end()
  // and we call Update.abort() on hash mismatch, the partition won't be
  // activated if verification fails.
  mbedtls_md_context_t shaCtx;
  mbedtls_md_init(&shaCtx);
  mbedtls_md_setup(&shaCtx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
  mbedtls_md_starts(&shaCtx);

  Update.onProgress(onOTAProgress);
  if (!Update.begin(contentLength, updateType)) {
    mbedtls_md_free(&shaCtx);
    return 503;
  }
  onOTAStart();

  WiFiClient *stream = http->getStreamPtr();
  uint8_t buf[1024];
  int bytesRead = 0;
  while (bytesRead < contentLength) {
    int toRead = std::min((int)sizeof(buf), contentLength - bytesRead);
    int r = stream->readBytes(buf, toRead);
    if (r <= 0) {
      yield();
      continue;
    }
    mbedtls_md_update(&shaCtx, buf, r);
    if (Update.write(buf, r) != (size_t)r) {
      mbedtls_md_free(&shaCtx);
      Update.abort();
      return 503;
    }
    bytesRead += r;
    yield();
  }

  uint8_t shaResult[32];
  mbedtls_md_finish(&shaCtx, shaResult);
  mbedtls_md_free(&shaCtx);

  char shaStr[65];
  for (int i = 0; i < 32; i++)
    sprintf(shaStr + (i * 2), "%02x", shaResult[i]);
  shaStr[64] = 0;
  if (expectedSHA256 != String(shaStr)) {
    Update.abort();
    return 503;
  }

  if (!Update.end() || !Update.isFinished())
    return 503;
  return 0;
}

// NOTE: an older updateWebUi() lived here. It was never wired into any code
// path (the only caller was commented out in runUpdate() above) and
// duplicated the checksum/download/Update.begin() logic already used by
// runUpdate(). Removed as part of the Phase 4 webserver split so there is
// exactly one OTA code path.

void onOTAError(ota_error_t error) {
  Wire.end();
  SPI.end();
  isOtaUpdating = false;
  delay(1000);
  ESP.restart();
}

void onOTAComplete() {
  Wire.end();
  SPI.end();
  delay(1000);
  ESP.restart();
}

bool getIsOTAUpdating() { return isOtaUpdating; }

String downloadSHA256(const String &sha256Url) {
  if (sha256Url.isEmpty())
    return "";

  auto http = HttpHelper::beginScoped(sha256Url);
  if (!http)
    return "";
  http->setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  int httpCode = http->GET();
  if (httpCode != HTTP_CODE_OK)
    return "";

  String sha256 = http->getString();
  sha256.trim();
  return sha256;
}
