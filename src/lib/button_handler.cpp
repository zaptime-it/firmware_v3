#include "button_handler.hpp"

TaskHandle_t buttonTaskHandle = NULL;
const TickType_t debounceDelay = pdMS_TO_TICKS(50);
TickType_t lastDebounceTime = 0;

#ifdef IS_BTCLOCK_V8
#define BTN_1 256
#define BTN_2 512
#define BTN_3 1024
#define BTN_4 2048
#else
#define BTN_1 2048
#define BTN_2 1024
#define BTN_3 512
#define BTN_4 256
#endif

void buttonTask(void *parameter) {
  while (1) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    
    TickType_t currentTime = xTaskGetTickCount();
    
    if ((currentTime - lastDebounceTime) >= debounceDelay) {
      lastDebounceTime = currentTime;
      
      std::lock_guard<std::mutex> lock(mcpMutex);
      
      if (!digitalRead(MCP_INT_PIN)) {
        uint16_t intFlags = mcp1.getInterruptFlagRegister();
        uint16_t intCap = mcp1.getInterruptCaptureRegister();
                
        // Check each button individually
        if (intFlags & BTN_1) handleButton1();
        if (intFlags & BTN_2) handleButton2();
        if (intFlags & BTN_3) handleButton3();
        if (intFlags & BTN_4) handleButton4();
      }
    }
    
    // Clear interrupt state
    while (!digitalRead(MCP_INT_PIN)) {
      std::lock_guard<std::mutex> lock(mcpMutex);
      mcp1.getInterruptCaptureRegister();
      delay(1);  // Small delay to prevent tight loop
    }
  }
}

// Helper functions to handle each button
void handleButton1() {
  toggleTimerActive();
}

void handleButton2() {
  nextScreen();
}

void handleButton3() {
  previousScreen();
}

void handleButton4() {
  showSystemStatusScreen();
}

void IRAM_ATTR handleButtonInterrupt() {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xTaskNotifyFromISR(buttonTaskHandle, 0, eNoAction, &xHigherPriorityTaskWoken);
  if (xHigherPriorityTaskWoken == pdTRUE) {
    portYIELD_FROM_ISR();
  }
}

void setupButtonTask() {
  xTaskCreate(buttonTask, "ButtonTask", 3072, NULL, tskIDLE_PRIORITY,
              &buttonTaskHandle);  // Create the FreeRTOS task
  // Use interrupt instead of task
  attachInterrupt(MCP_INT_PIN, handleButtonInterrupt, CHANGE);
  Serial.printf("Button task created\n");
}
