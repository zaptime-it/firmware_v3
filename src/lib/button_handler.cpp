#include "button_handler.hpp"

// Initialize static members
TaskHandle_t ButtonHandler::buttonTaskHandle = NULL;
ButtonState ButtonHandler::buttonStates[4] = {};

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

void ButtonHandler::buttonTask(void *parameter) {
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        
        TickType_t currentTime = xTaskGetTickCount();
        
        std::lock_guard<std::mutex> lock(mcpMutex);
        
        if (!digitalRead(MCP_INT_PIN)) {
            uint16_t intFlags = mcp1.getInterruptFlagRegister();
            uint16_t intCap = mcp1.getInterruptCaptureRegister();
            
            // Check button states
            if (intFlags & BTN_1) handleButtonPress(0);
            if (intFlags & BTN_2) handleButtonPress(1);
            if (intFlags & BTN_3) handleButtonPress(2);
            if (intFlags & BTN_4) handleButtonPress(3);
            
            // Check for button releases
            for (int i = 0; i < 4; i++) {
                if (buttonStates[i].isPressed) {
                    bool currentlyPressed = false;
                    switch (i) {
                        case 0: currentlyPressed = (intCap & BTN_1); break;
                        case 1: currentlyPressed = (intCap & BTN_2); break;
                        case 2: currentlyPressed = (intCap & BTN_3); break;
                        case 3: currentlyPressed = (intCap & BTN_4); break;
                    }
                    if (!currentlyPressed) {
                        handleButtonRelease(i);
                    }
                }
            }
        }
        
        // Clear interrupt state
        while (!digitalRead(MCP_INT_PIN)) {
            mcp1.getInterruptCaptureRegister();
            delay(1);
        }
    }
}

void ButtonHandler::handleButtonPress(int buttonIndex) {
    TickType_t currentTime = xTaskGetTickCount();
    ButtonState &state = buttonStates[buttonIndex];
    
    if ((currentTime - state.lastPressTime) >= debounceDelay) {
        state.isPressed = true;
        state.lastPressTime = currentTime;
    }
}

void ButtonHandler::handleButtonRelease(int buttonIndex) {
    ButtonState &state = buttonStates[buttonIndex];
    
    if (!state.isPressed) return;  // Ignore if button wasn't pressed
    
    state.isPressed = false;
    handleSingleClick(buttonIndex);
}

void ButtonHandler::handleSingleClick(int buttonIndex) {
    switch (buttonIndex) {
        case 0:
            toggleTimerActive();
            break;
        case 1:
            ScreenHandler::nextScreen();
            break;
        case 2:
            ScreenHandler::previousScreen();
            break;
        case 3:
            ScreenHandler::showSystemStatusScreen();
            break;
    }
}

void IRAM_ATTR ButtonHandler::handleButtonInterrupt() {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xTaskNotifyFromISR(buttonTaskHandle, 0, eNoAction, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

void ButtonHandler::setup() {
    xTaskCreate(buttonTask, "ButtonTask", 3072, NULL, tskIDLE_PRIORITY,
                &buttonTaskHandle);
    attachInterrupt(MCP_INT_PIN, handleButtonInterrupt, FALLING);
}
