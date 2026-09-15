#pragma once
#include <Arduino.h>

class ButtonHandler
{
public:
    ButtonHandler(uint8_t pin, uint32_t debounceMs);

    void init();

    bool pollPressed();

private:
    static void IRAM_ATTR isr();

    static volatile bool pressedFlag_;

    const uint8_t pin_;
    const uint32_t debounceMs_;
    uint32_t lastDebounceTime_;
};
