#include "ota.hpp"
#include "lib/drivers/leds/led_handler.hpp"
#include "lib/system/pref_keys.hpp"

#include <algorithm>
#include <mutex>

#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"

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

// Read the staged OTA partition back and verify its first `len` bytes match
// expectedSHA256. esp_https_ota validates the image header (magic, chip rev,
// CRC, optional secure-boot signature) but does NOT check against an external
// hash from the release manifest — that integrity check is still ours to do.
static bool verifyStagedPartitionSHA256(const esp_partition_t *part, size_t len,
                                        const String &expected) {
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
  mbedtls_md_starts(&ctx);

  uint8_t buf[1024];
  size_t off = 0;
  while (off < len) {
    size_t toRead = std::min(sizeof(buf), len - off);
    if (esp_partition_read(part, off, buf, toRead) != ESP_OK) {
      mbedtls_md_free(&ctx);
      return false;
    }
    mbedtls_md_update(&ctx, buf, toRead);
    off += toRead;
  }

  uint8_t result[32];
  mbedtls_md_finish(&ctx, result);
  mbedtls_md_free(&ctx);

  char hex[65];
  for (int i = 0; i < 32; i++)
    sprintf(hex + (i * 2), "%02x", result[i]);
  hex[64] = 0;
  return expected.equalsIgnoreCase(hex);
}

// Firmware OTA over HTTPS via IDF's esp_https_ota — handles handshake,
// per-chunk write, partition staging and image-header validation internally.
// Lets us drop the manual HTTPClient + Update.write loop and the per-request
// CA-bundle re-attach workaround from the old code path.
static int downloadFirmwareViaEspHttpsOta(const String &url,
                                          const String &expectedSHA256) {
  // Hold tls_gate's mutex for the whole upgrade. esp_https_ota runs its own
  // mbedtls context; if a data-source HTTPS poller or WS reconnect starts a
  // parallel handshake the second one can OOM or trip the heap-poisoning
  // assert from concurrent SSL contexts. Same rationale as
  // HttpHelper::beginScoped — see shared.hpp.
  std::unique_lock<std::mutex> tlsLock(tls_gate::mutex());

  // Pick the staging partition explicitly so we can hash it after the
  // download but before esp_https_ota_finish flips the boot pointer.
  const esp_partition_t *staging = esp_ota_get_next_update_partition(NULL);
  if (!staging)
    return 503;

  esp_http_client_config_t http_config = {};
  http_config.url = url.c_str();
  http_config.crt_bundle_attach = esp_crt_bundle_attach;
  http_config.timeout_ms = 30000;
  http_config.keep_alive_enable = true;

  esp_https_ota_config_t ota_config = {};
  ota_config.http_config = &http_config;

  esp_https_ota_handle_t handle = NULL;
  if (esp_https_ota_begin(&ota_config, &handle) != ESP_OK)
    return 503;

  onOTAStart();

  int total = esp_https_ota_get_image_size(handle);
  esp_err_t err;
  do {
    err = esp_https_ota_perform(handle);
    if (err == ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
      int got = esp_https_ota_get_image_len_read(handle);
      onOTAProgress(got, total > 0 ? total : got + 1);
    }
  } while (err == ESP_ERR_HTTPS_OTA_IN_PROGRESS);

  if (err != ESP_OK ||
      !esp_https_ota_is_complete_data_received(handle)) {
    esp_https_ota_abort(handle);
    return 503;
  }

  int actualLen = esp_https_ota_get_image_len_read(handle);
  if (actualLen <= 0 ||
      !verifyStagedPartitionSHA256(staging, actualLen, expectedSHA256)) {
    esp_https_ota_abort(handle);
    return 503;
  }

  if (esp_https_ota_finish(handle) != ESP_OK)
    return 503;
  return 0;
}

// WebUI OTA (download a .bin from a release URL and write it to the LittleFS
// partition). esp_https_ota only targets app partitions, so the WebUI path
// stays on Update.h with U_SPIFFS. Streams through mbedtls in the same loop
// as Update.write so we never buffer the full 1.5 MB in RAM.
static int downloadWebUiViaUpdate(const String &url,
                                  const String &expectedSHA256) {
  auto http = HttpHelper::beginScoped(url);
  if (!http)
    return 503;
  http->setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  int httpCode = http->GET();
  if (httpCode != HTTP_CODE_OK)
    return 503;

  int contentLength = http->getSize();
  if (contentLength <= 0)
    return 503;

  mbedtls_md_context_t shaCtx;
  mbedtls_md_init(&shaCtx);
  mbedtls_md_setup(&shaCtx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
  mbedtls_md_starts(&shaCtx);

  Update.onProgress(onOTAProgress);
  if (!Update.begin(contentLength, UPDATE_WEBUI)) {
    mbedtls_md_free(&shaCtx);
    return 503;
  }
  onOTAStart();

  NetworkClient *stream = http->getStreamPtr();
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
  if (!expectedSHA256.equalsIgnoreCase(shaStr)) {
    Update.abort();
    return 503;
  }

  if (!Update.end() || !Update.isFinished())
    return 503;
  return 0;
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

  if (latestRelease.fileUrl.isEmpty() || latestRelease.checksumUrl.isEmpty())
    return 503;

  String expectedSHA256 = downloadSHA256(latestRelease.checksumUrl);
  if (expectedSHA256.isEmpty())
    return 503;
  expectedSHA256.toLowerCase();

  if (updateType == UPDATE_FIRMWARE)
    return downloadFirmwareViaEspHttpsOta(latestRelease.fileUrl,
                                          expectedSHA256);
  return downloadWebUiViaUpdate(latestRelease.fileUrl, expectedSHA256);
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
