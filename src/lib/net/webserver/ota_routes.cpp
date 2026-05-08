#include "internal.hpp"

#include "esp_partition.h"
#include "lib/system/shared.hpp"
#include <Update.h>

// Completion callback invoked after the multipart upload body for
// /upload/firmware or /upload/webui has been fully handed to Update.write().
// The response body handler fires independently of the streaming body
// handler, so auth needs to be re-checked here.
static void onFirmwareUpdate(AsyncWebServerRequest *request) {
  if (requireHttpAuth(request))
    return;

  const bool shouldReboot = !Update.hasError();
  if (shouldReboot) {
    // Reboot after the response is flushed to the client. Schedule via a
    // dedicated task so we don't stall the AsyncTCP callback.
    request->onDisconnect([]() { scheduleDelayedRestart(500); });

    if (events.count())
      events.send("closing");
  }

  AsyncWebServerResponse *response = request->beginResponse(
      HTTP_OK, "text/plain", shouldReboot ? "OK" : "FAIL");
  response->addHeader("Connection", "close");
  request->send(response);
}

static void onAutoUpdateFirmware(AsyncWebServerRequest *request) {
  if (requireHttpAuth(request))
    return;
  UpdateMessage msg = {UPDATE_ALL};
  if (xQueueSend(otaQueue, &msg, 0) == pdTRUE) {
    request->send(HTTP_OK, "application/json",
                  "{\"msg\":\"Firmware update triggered\"}");
  } else {
    request->send(HTTP_SERVICE_UNAVAILABLE, "application/json",
                  "{\"msg\":\"Update already in progress\"}");
  }
}

// Streaming body handler shared by the firmware and webui upload endpoints.
// The split is purely which esp_ota partition to target (U_FLASH vs
// U_SPIFFS); everything else is identical.
static void asyncFileUpdateHandler(AsyncWebServerRequest *request,
                                   String filename, size_t index, uint8_t *data,
                                   size_t len, bool final, int command) {
  // The LittleFS partition size on some boards (e.g. lolin_s3_mini: 0x66C00)
  // is not a multiple of SPI_FLASH_SEC_SIZE. Update.begin() accepts a smaller
  // size, but Update.write() aborts with "Not Enough Space" the moment the
  // cumulative byte count exceeds that size. We therefore track the capped
  // size per upload and only hand the library bytes it will accept; the
  // remaining trailing bytes are always 0xFF padding in the generated image
  // and are ignored.
  static size_t s_fsCapSize = 0;
  static size_t s_fsWritten = 0;

  if (!index) {
    if (command == U_FLASH) {
      // The original expression had the closing paren misplaced, so the
      // command argument was actually consumed by the comma operator and
      // Update.begin() was called with only the size. Put command back
      // inside the Update.begin() call.
      if (!Update.begin(((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000),
                        command)) {
        Update.printError(Serial);
        return;
      }
    } else if (command == U_SPIFFS) {
      // Unmount LittleFS so the flash driver has exclusive access to the
      // partition for the OTA erase/write cycle.
      LittleFS.end();

      // The Arduino Update library asks esp_partition_erase_range() to erase
      // a full SPI_FLASH_SEC_SIZE (4KB) sector at the tail of the partition.
      // If partition->size is not a multiple of 4KB (e.g. 0x66C00 on the
      // lolin_s3_mini 4MB partition table), that erase overruns the partition
      // and IDF returns ESP_ERR_INVALID_SIZE; Update then aborts the whole
      // upload with "Flash Erase Failed" (boards with 4KB-aligned sizes such
      // as btclock_rev_b/0xCD000 or btclock_v8/0x200000 are unaffected).
      // The last sub-sector of the littlefs image is always 0xFF padding, so
      // rounding the update size down to the sector boundary is safe.
      size_t fsSize = UPDATE_SIZE_UNKNOWN;
      const esp_partition_t *fsPart = esp_partition_find_first(
          ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, NULL);
      if (fsPart)
        fsSize = fsPart->size & ~(SPI_FLASH_SEC_SIZE - 1);
      s_fsCapSize = fsSize;
      s_fsWritten = 0;

      if (!Update.begin(fsSize, U_SPIFFS)) {
        Update.printError(Serial);
        return;
      }
    }
  }
  if (!Update.hasError()) {
    size_t toWrite = len;
    if (command == U_SPIFFS && s_fsCapSize > 0) {
      // Only hand Update.write() bytes that still fit in the capped size;
      // the remaining trailing bytes (0xFF padding in the image) are
      // silently discarded so we don't trip UPDATE_ERROR_SPACE.
      size_t remaining =
          (s_fsWritten >= s_fsCapSize) ? 0 : (s_fsCapSize - s_fsWritten);
      if (toWrite > remaining)
        toWrite = remaining;
      s_fsWritten += len;
    }
    if (toWrite > 0 && Update.write(data, toWrite) != toWrite) {
      Update.printError(Serial);
    }
  }
  if (final) {
    if (Update.end(true)) {
      onApiRestart(request);
    } else {
      Update.printError(Serial);
    }
  }
}

static void asyncWebuiUpdateHandler(AsyncWebServerRequest *request,
                                    String filename, size_t index,
                                    uint8_t *data, size_t len, bool final) {
  if (index == 0 && requireHttpAuth(request))
    return;
  asyncFileUpdateHandler(request, filename, index, data, len, final, U_SPIFFS);
}

static void asyncFirmwareUpdateHandler(AsyncWebServerRequest *request,
                                       String filename, size_t index,
                                       uint8_t *data, size_t len, bool final) {
  if (index == 0 && requireHttpAuth(request))
    return;
  asyncFileUpdateHandler(request, filename, index, data, len, final, U_FLASH);
}

void registerOtaRoutes() {
  // Guarded: when OTA is disabled in preferences, the upload routes are
  // simply not registered so the WebUI's upload form returns 404 instead of
  // silently accepting uploads that will be rejected by Update.begin().
  if (!preferences.getBool("otaEnabled", DEFAULT_OTA_ENABLED))
    return;

  server.on("/upload/firmware", AsyncWebRequestMethod::HTTP_POST, onFirmwareUpdate,
            asyncFirmwareUpdateHandler);
  server.on("/upload/webui", AsyncWebRequestMethod::HTTP_POST, onFirmwareUpdate,
            asyncWebuiUpdateHandler);
  server.on("/api/firmware/auto_update", AsyncWebRequestMethod::HTTP_POST, onAutoUpdateFirmware);
}
