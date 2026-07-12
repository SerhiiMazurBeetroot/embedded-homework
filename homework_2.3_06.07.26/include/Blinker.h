#pragma once
#include <Arduino.h>
#include "Led.h"

class Blinker
{
public:
    Blinker(uint8_t pin, uint32_t intervalMs) : led_(pin), interval_(intervalMs), prevMs_(0) {}

    void init() { led_.init(); }
    void update(unsigned long currentMs);

private:
    Led led_;
    uint32_t interval_;
    unsigned long prevMs_;
};
