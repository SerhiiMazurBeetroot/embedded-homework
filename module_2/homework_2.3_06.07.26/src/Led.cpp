#include "Led.h"

void Led::init()
{
    pinMode(pin_, OUTPUT);
    set(LedState::Off);
}

void Led::set(LedState state)
{
    state_ = state;
    digitalWrite(pin_, static_cast<uint8_t>(state_));
}

void Led::toggle()
{
    set(state_ == LedState::On ? LedState::Off : LedState::On);
}
