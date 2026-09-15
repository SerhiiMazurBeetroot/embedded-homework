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
    explicit Led(uint8_t pin) : pin_(pin), state_(LedState::Off) {}

    void init();
    void set(LedState state);
    void toggle();
    LedState state() const { return state_; }

private:
    const uint8_t pin_;
    LedState state_;
};
