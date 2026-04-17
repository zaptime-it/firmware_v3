#include "timers.hpp"
#include "led_handler.hpp"

esp_timer_handle_t screenRotateTimer;
esp_timer_handle_t minuteTimer;

// Cached for use from minuteTimerISR. Accessing singleton instance methods
// from an IRAM_ATTR ISR is unsafe because the flash cache may be disabled.
static volatile TaskHandle_t s_bitaxeIsrHandle = nullptr;
static volatile TaskHandle_t s_miningPoolIsrHandle = nullptr;

void setBitaxeTaskHandleForIsr(TaskHandle_t handle) { s_bitaxeIsrHandle = handle; }
void setMiningPoolTaskHandleForIsr(TaskHandle_t handle) { s_miningPoolIsrHandle = handle; }

void setupTimeUpdateTimer(void *pvParameters) {
  const esp_timer_create_args_t minuteTimerConfig = {
      .callback = &minuteTimerISR, .name = "minute_timer"};

  esp_timer_create(&minuteTimerConfig, &minuteTimer);

  time_t currentTime;
  struct tm timeinfo;
  time(&currentTime);
  localtime_r(&currentTime, &timeinfo);
  uint32_t secondsUntilNextMinute = 60 - timeinfo.tm_sec;

  if (secondsUntilNextMinute > 0)
    vTaskDelay(pdMS_TO_TICKS((secondsUntilNextMinute * 1000)));

  esp_timer_start_periodic(minuteTimer, usPerMinute);

  WorkItem timeUpdate = {TASK_TIME_UPDATE, 0};
  xQueueSend(workQueue, &timeUpdate, portMAX_DELAY);
  //    xTaskNotifyGive(timeUpdateTaskHandle);

  vTaskDelete(NULL);
}

void setupScreenRotateTimer(void *pvParameters) {
  const esp_timer_create_args_t screenRotateTimerConfig = {
      .callback = &screenRotateTimerISR, .name = "screen_rotate_timer"};

  esp_timer_create(&screenRotateTimerConfig, &screenRotateTimer);

  if (preferences.getBool("timerActive", DEFAULT_TIMER_ACTIVE)) {
    esp_timer_start_periodic(screenRotateTimer,
                             getTimerSeconds() * usPerSecond);
  }

  vTaskDelete(NULL);
}

uint getTimerSeconds() { return preferences.getUInt("timerSeconds", DEFAULT_TIMER_SECONDS); }

// Hardware esp_timer state is the source of truth; the NVS `timerActive`
// flag is only the persisted last-user-intent consulted on boot by
// setupScreenRotateTimer(). Runtime writers (setTimerActive) update both
// so the two never drift outside the transient stealFocus stop/restart
// window in BlockNotify.
bool isTimerActive() { return esp_timer_is_active(screenRotateTimer); }

void setTimerActive(bool status) {
  if (status) {
    esp_timer_start_periodic(screenRotateTimer,
                             getTimerSeconds() * usPerSecond);
    getLedHandler().queueEffect(LED_EFFECT_START_TIMER);
    preferences.putBool("timerActive", true);
  } else {
    esp_timer_stop(screenRotateTimer);
    getLedHandler().queueEffect(LED_EFFECT_PAUSE_TIMER);
    preferences.putBool("timerActive", false);
  }

  if (eventSourceTaskHandle != NULL) xTaskNotifyGive(eventSourceTaskHandle);
}

void toggleTimerActive() { setTimerActive(!isTimerActive()); }

void IRAM_ATTR minuteTimerISR(void *arg) {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  WorkItem timeUpdate = {TASK_TIME_UPDATE, 0};
  xQueueSendFromISR(workQueue, &timeUpdate, &xHigherPriorityTaskWoken);

  TaskHandle_t bitaxeHandle = s_bitaxeIsrHandle;
  if (bitaxeHandle != NULL) {
    vTaskNotifyGiveFromISR(bitaxeHandle, &xHigherPriorityTaskWoken);
  }

  TaskHandle_t miningPoolHandle = s_miningPoolIsrHandle;
  if (miningPoolHandle != NULL) {
    vTaskNotifyGiveFromISR(miningPoolHandle, &xHigherPriorityTaskWoken);
  }

  if (xHigherPriorityTaskWoken == pdTRUE) {
    portYIELD_FROM_ISR();
  }
}

void IRAM_ATTR screenRotateTimerISR(void *arg) {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  TaskHandle_t handle = taskScreenRotateTaskHandle;
  if (handle != NULL) {
    vTaskNotifyGiveFromISR(handle, &xHigherPriorityTaskWoken);
  }
  if (xHigherPriorityTaskWoken == pdTRUE) {
    portYIELD_FROM_ISR();
  }
}