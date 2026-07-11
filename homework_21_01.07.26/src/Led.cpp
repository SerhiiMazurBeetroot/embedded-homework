#include "Led.h"
#include <Arduino.h>

Led::Led(uint8_t pin) : pin_(pin), state_(LedState::Off) {}

void Led::init()
{
    pinMode(pin_, OUTPUT);
    set(LedState::Off);
}

void Led::set(LedState state)
{
    state_ = state;
    digitalWrite(pin_, state_ == LedState::On ? HIGH : LOW);
}

void Led::toggle()
{
    set(state_ == LedState::On ? LedState::Off : LedState::On);
}
