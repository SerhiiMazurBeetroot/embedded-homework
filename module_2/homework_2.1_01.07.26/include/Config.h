#pragma once
#include <Arduino.h>

class Config
{
public:
    static constexpr uint8_t LED_PIN = 4;
    static constexpr uint8_t BUTTON_PIN = 8;

    static constexpr uint32_t BLINK_INTERVAL_MS = 500;
    static constexpr uint32_t DEBOUNCE_MS = 50;
    static constexpr uint32_t ITERATIONS_TO_REPORT = 1000;

    static const uint8_t SHORT_PRESS_BLINK_COUNT = 3;
};
