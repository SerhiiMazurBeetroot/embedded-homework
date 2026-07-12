#pragma once
#include <Arduino.h>

struct LedConfig
{
    uint8_t pin;
    uint32_t intervalMs;
};

class Config
{
public:
    static constexpr LedConfig LEDS[] = {
        {4, 200},  // LED1
        {5, 500},  // LED2
        {6, 1000}, // LED3
    };

    static constexpr size_t LED_COUNT = sizeof(LEDS) / sizeof(LEDS[0]);
};
