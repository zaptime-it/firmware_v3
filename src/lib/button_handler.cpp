#include "button_handler.hpp"

// Initialize static members
TaskHandle_t ButtonHandler::buttonTaskHandle = NULL;
ButtonState ButtonHandler::buttonStates[4] = {};

void ButtonHandler::buttonTask(void *parameter) {
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // Snapshot MCP state under the lock, then release before dispatching
        // handlers or polling INT. Holding mcpMutex across delay(1) used to
        // block every other MCP user (config, i2c bus) for unbounded time.
        uint16_t intFlags = 0;
        uint16_t intCap = 0;
        bool intAsserted = false;
        {
            std::lock_guard<std::mutex> lock(mcpMutex);
            if (!digitalRead(MCP_INT_PIN)) {
                intFlags = mcp1.getInterruptFlagRegister();
                intCap = mcp1.getInterruptCaptureRegister();
                intAsserted = true;
            }
        }

        if (intAsserted) {
            if (intFlags & BTN_1) handleButtonPress(0);
            if (intFlags & BTN_2) handleButtonPress(1);
            if (intFlags & BTN_3) handleButtonPress(2);
            if (intFlags & BTN_4) handleButtonPress(3);

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

        // Poll INT line to clear the capture register; re-acquire the mutex
        // only for the actual MCP read and release it between iterations.
        while (!digitalRead(MCP_INT_PIN)) {
            {
                std::lock_guard<std::mutex> lock(mcpMutex);
                mcp1.getInterruptCaptureRegister();
            }
            vTaskDelay(pdMS_TO_TICKS(1));
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
    if (preferences.getBool("inverseButtons", DEFAULT_INVERSE_BUTTONS)) {
        buttonIndex = 3 - buttonIndex;
    }

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
