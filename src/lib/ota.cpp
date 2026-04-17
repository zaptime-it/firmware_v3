#include "ota.hpp"
#include "led_handler.hpp"

TaskHandle_t taskOtaHandle = NULL;
bool isOtaUpdating = false;
QueueHandle_t otaQueue;



void setupOTA()
{
  if (preferences.getBool("otaEnabled", DEFAULT_OTA_ENABLED))
  {
    ArduinoOTA.onStart(onOTAStart);

    ArduinoOTA.onProgress(onOTAProgress);
    ArduinoOTA.onError(onOTAError);
    ArduinoOTA.onEnd(onOTAComplete);

    ArduinoOTA.setHostname(getMyHostname().c_str());
    ArduinoOTA.setMdnsEnabled(false);
    ArduinoOTA.setRebootOnSuccess(false);
    ArduinoOTA.begin();
    // downloadUpdate();
    otaQueue = xQueueCreate(1, sizeof(UpdateMessage));

    xTaskCreate(handleOTATask, "handleOTA", 8192, NULL, 20,
                &taskOtaHandle);
  }
}

void onOTAProgress(unsigned int progress, unsigned int total)
{
  // Guard against div-by-zero: if total is < 100 the previous expression
  // `total / 100` evaluated to 0 and triggered a panic.
  uint percentage = 0;
  if (total >= 100)
  {
    percentage = progress / (total / 100);
  }
  else if (total > 0)
  {
    percentage = (progress * 100) / total;
  }
  auto& ledHandler = getLedHandler();
  auto& pixels = ledHandler.getPixels();
  
  pixels.fill(pixels.Color(0, 255, 0));
  if (percentage < 100)
  {
    pixels.setPixelColor(0, pixels.Color(0, 0, 0));
  }
  if (percentage < 75)
  {
    pixels.setPixelColor(1, pixels.Color(0, 0, 0));
  }
  if (percentage < 50)
  {
    pixels.setPixelColor(2, pixels.Color(0, 0, 0));
  }
  if (percentage < 25)
  {
    pixels.setPixelColor(3, pixels.Color(0, 0, 0));
  }
  pixels.show();
}

void onOTAStart()
{
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
  auto& blockNotify = BlockNotify::getInstance();
  blockNotify.stop();
}

void handleOTATask(void *parameter)
{
  UpdateMessage msg;

  for (;;)
  {
    if (xQueueReceive(otaQueue, &msg, 0) == pdTRUE)
    {
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

ReleaseInfo getLatestRelease(const String &fileToDownload)
{
  String releaseUrl = preferences.getString("gitReleaseUrl");
  WiFiClientSecure client;
//  client.setCACert(isrg_root_x1cert);
  client.setCACertBundle(rootca_crt_bundle_start);


  HTTPClient http;
  http.begin(client, releaseUrl);
  http.setUserAgent(USER_AGENT);

  int httpCode = http.GET();

  ReleaseInfo info = {"", ""};

  if (httpCode == HTTP_CODE_OK)
  {
    String payload = http.getString();

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (err)
    {
      Serial.printf("getLatestRelease: JSON parse error: %s\r\n", err.c_str());
      http.end();
      return info;
    }

    JsonArray assets = doc["assets"];
    if (assets.isNull())
    {
      Serial.println(F("getLatestRelease: no 'assets' array in response"));
      http.end();
      return info;
    }

    for (JsonObject asset : assets)
    {
      String assetName = asset["name"].as<String>();
      if (assetName == fileToDownload)
      {
        info.fileUrl = asset["browser_download_url"].as<String>();
      }
      else if (assetName == fileToDownload + ".sha256")
      {
        info.checksumUrl = asset["browser_download_url"].as<String>();
      }

      if (!info.fileUrl.isEmpty() && !info.checksumUrl.isEmpty())
      {
        break;
      }
    }
    Serial.printf("Latest release URL: %s\r\n", info.fileUrl.c_str());
    Serial.printf("Checksum URL: %s\r\n", info.checksumUrl.c_str());
  }
  else
  {
    Serial.printf("getLatestRelease: HTTP error: %d\r\n", httpCode);
  }
  http.end();
  return info;
}

int downloadUpdateHandler(char updateType)
{
  WiFiClientSecure client;
  client.setCACertBundle(rootca_crt_bundle_start);
  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  ReleaseInfo latestRelease;

  switch (updateType)
  {
  case UPDATE_FIRMWARE:
  {
    latestRelease = getLatestRelease(getFirmwareFilename());
  }
  break;
  case UPDATE_WEBUI:
  {
    latestRelease = getLatestRelease(getWebUiFilename());
    // updateWebUi(latestRelease.fileUrl, U_SPIFFS);
    // return 0;
  }
  break;
  }

  // Bail if the release metadata didn't resolve to any URLs.
  if (latestRelease.fileUrl.isEmpty() || latestRelease.checksumUrl.isEmpty())
  {
    Serial.println(F("No release artifacts found. Aborting update."));
    return 503;
  }

  // First, download the expected SHA256
  String expectedSHA256 = downloadSHA256(latestRelease.checksumUrl);
  if (expectedSHA256.isEmpty())
  {
    Serial.println(F("Failed to get SHA256 checksum. Aborting update."));
    return 503;
  }

  http.begin(client, latestRelease.fileUrl);
  http.setUserAgent(USER_AGENT);

  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK)
  {
    int contentLength = http.getSize();
    if (contentLength > 0)
    {
      // Allocate memory to store the firmware
      uint8_t *firmware = (uint8_t *)malloc(contentLength);
      if (!firmware)
      {
        Serial.println(F("Not enough memory to store firmware"));
        return 503;
      }

      WiFiClient *stream = http.getStreamPtr();
      size_t bytesRead = 0;
      while (bytesRead < contentLength)
      {
        size_t available = stream->available();
        if (available)
        {
          size_t readBytes = stream->readBytes(firmware + bytesRead, available);
          bytesRead += readBytes;
        }
        yield(); // Allow background tasks to run
      }

      if (bytesRead != contentLength)
      {
        Serial.println(F("Failed to read entire firmware"));
        free(firmware);
        return 503;
      }

      // Calculate SHA256
      String calculated_sha256 = calculateSHA256(firmware, contentLength);

      Serial.print(F("Calculated checksum: "));
      Serial.println(calculated_sha256);
      Serial.print(F("Expected checksum:   "));
      Serial.println(expectedSHA256);

      if (calculated_sha256 != expectedSHA256)
      {
        Serial.println(F("Checksum mismatch. Aborting update."));
        free(firmware);
        return 503;
      }
      
      Update.onProgress(onOTAProgress);

      if (Update.begin(contentLength, updateType))
      {
        onOTAStart();
        size_t written = Update.write(firmware, contentLength);

        // Free the download buffer immediately after Update.write(); Update's
        // internal copy has taken ownership of the bytes that matter. NULL the
        // pointer so subsequent error branches don't double-free.
        free(firmware);
        firmware = nullptr;

        if (written != contentLength)
        {
          Serial.println("Written only : " + String(written) + "/" + String(contentLength) + ". Retry?");
          Update.abort();
          return 503;
        }
        Serial.println("Written : " + String(written) + " successfully");

        if (Update.end())
        {
          Serial.println(F("OTA done!"));
          if (Update.isFinished())
          {
            Serial.println(F("Update successfully completed. Rebooting."));
//            ESP.restart();
          }
          else
          {
            Serial.println(F("Update not finished? Something went wrong!"));
            return 503;
          }
        }
        else
        {
          Serial.println("Error Occurred. Error #: " + String(Update.getError()));
          return 503;
        }
      }
      else
      {
        Serial.println(F("Not enough space to begin OTA"));
        free(firmware);
        firmware = nullptr;
        return 503;
      }
    }
    else
    {
      Serial.println(F("Invalid content length"));
      return 503;
    }
  }
  else
  {
    Serial.printf("HTTP error: %d\n", httpCode);
    return 503;
  }
  http.end();

  return 0;
}

void updateWebUi(String latestRelease, int command)
{
  WiFiClientSecure client;
  client.setCACertBundle(rootca_crt_bundle_start);
  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.begin(client, latestRelease);
  http.setUserAgent(USER_AGENT);

  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK)
  {
    int contentLength = http.getSize();
    if (contentLength > 0)
    {
      uint8_t *buffer = (uint8_t *)malloc(contentLength);
      if (buffer)
      {
        WiFiClient *stream = http.getStreamPtr();
        size_t written = stream->readBytes(buffer, contentLength);

        if (written == contentLength)
        {
          String expectedSHA256 = "";
          if (command == U_FLASH)
          {
            expectedSHA256 = downloadSHA256(getFirmwareFilename());
            Serial.print("Expected checksum:   ");
            Serial.println(expectedSHA256);
          }

          String calculated_sha256 = calculateSHA256(buffer, contentLength);
          Serial.print("Checksum is ");
          Serial.println(calculated_sha256);
          if ((command == U_FLASH && expectedSHA256.equals(calculated_sha256)) || command == U_SPIFFS)
          {
            Serial.println(F("Checksum verified. Proceeding with update."));

            Update.onProgress(onOTAProgress);

            if (Update.begin(contentLength, command))
            {
              onOTAStart();

              Update.write(buffer, contentLength);
              if (Update.end())
              {
                Serial.println(F("Update complete. Rebooting."));
                ESP.restart();
              }
              else
              {
                Serial.println(F("Error in update process."));
              }
            }
            else
            {
              Serial.println(F("Not enough space to begin OTA"));
            }
          }
          else
          {
            Serial.println(F("Checksum mismatch. Aborting update."));
          }
        }
        else
        {
          Serial.println(F("Error downloading firmware"));
        }
        free(buffer);
      }
      else
      {
        Serial.println(F("Not enough memory to allocate buffer"));
      }
    }
    else
    {
      Serial.println(F("Invalid content length"));
    }
  }
  else
  {
    Serial.print(httpCode);
    Serial.println("Error on HTTP request");
  }
}

void onOTAError(ota_error_t error)
{
  Serial.println(F("\nOTA update error, restarting"));
  Wire.end();
  SPI.end();
  isOtaUpdating = false;
  delay(1000);
  ESP.restart();
}

void onOTAComplete()
{
  Serial.println(F("\nOTA update finished"));
  Wire.end();
  SPI.end();
  delay(1000);
  ESP.restart();
}

bool getIsOTAUpdating()
{
  return isOtaUpdating;
}

String downloadSHA256(const String &sha256Url)
{
  if (sha256Url.isEmpty())
  {
    Serial.println(F("Failed to get SHA256 file URL"));
    return "";
  }

  WiFiClientSecure client;
  client.setCACertBundle(rootca_crt_bundle_start);
  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.begin(client, sha256Url);
  http.setUserAgent(USER_AGENT);

  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK)
  {
    String sha256 = http.getString();
    sha256.trim(); // Remove any whitespace or newline characters
    return sha256;
  }
  else
  {
    Serial.printf("Failed to download SHA256 file. HTTP error: %d\n", httpCode);
    return "";
  }
}