#pragma once

#include <Arduino.h>

enum class LedState : uint8_t
{
    Off = 0,
    On = 1
};

class Led
{

public:
    explicit Led(uint8_t pin);

    void init();

    void set(LedState state);

    LedState get() const { return state_; }

    void toggle();

private:
    const uint8_t pin_;
    LedState state_;
};
