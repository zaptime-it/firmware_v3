#include "button_handler.hpp"

TaskHandle_t buttonTaskHandle = NULL;
ButtonState buttonStates[4];

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
            
            // Check for long press on pressed buttons
            for (int i = 0; i < 4; i++) {
                if (buttonStates[i].isPressed && !buttonStates[i].longPressHandled) {
                    if ((currentTime - buttonStates[i].pressStartTime) >= longPressDelay) {
                        handleLongPress(i);
                        buttonStates[i].longPressHandled = true;
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

void handleButtonPress(int buttonIndex) {
    TickType_t currentTime = xTaskGetTickCount();
    ButtonState &state = buttonStates[buttonIndex];
    
    if ((currentTime - state.lastPressTime) >= debounceDelay) {
        state.isPressed = true;
        state.pressStartTime = currentTime;
        state.longPressHandled = false;
        
        // Check for double click
        if ((currentTime - state.lastPressTime) <= doubleClickDelay) {
            state.clickCount++;
            if (state.clickCount == 2) {
                handleDoubleClick(buttonIndex);
                state.clickCount = 0;
            }
        } else {
            state.clickCount = 1;
        }
        
        state.lastPressTime = currentTime;
    }
}

void handleButtonRelease(int buttonIndex) {
    TickType_t currentTime = xTaskGetTickCount();
    ButtonState &state = buttonStates[buttonIndex];
    
    state.isPressed = false;
    
    // If this wasn't a long press or double click, handle as single click
    if (!state.longPressHandled && state.clickCount == 1 && 
        (currentTime - state.pressStartTime) < longPressDelay) {
        handleSingleClick(buttonIndex);
        state.clickCount = 0;
    }
}

// Button action handlers
void handleSingleClick(int buttonIndex) {
    switch (buttonIndex) {
        case 0:
            toggleTimerActive();
            break;
        case 1:
            Serial.println("Button 2 single click");
            ScreenHandler::nextScreen();
            break;
        case 2:
            Serial.println("Button 3 single click");
            ScreenHandler::previousScreen();
            break;
        case 3:
            ScreenHandler::showSystemStatusScreen();
            break;
    }
}

void handleDoubleClick(int buttonIndex) {
    Serial.printf("Button %d double clicked\n", buttonIndex + 1);
}

void handleLongPress(int buttonIndex) {
    Serial.printf("Button %d long press detected\n", buttonIndex + 1);
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
                &buttonTaskHandle);
    attachInterrupt(MCP_INT_PIN, handleButtonInterrupt, FALLING);
}
