#include "ButtonHandler.h"
#include <Arduino.h>

volatile bool ButtonHandler::pressedFlag_ = false;

ButtonHandler::ButtonHandler(uint8_t pin, uint32_t debounceMs)
    : pin_(pin), debounceMs_(debounceMs), lastDebounceTime_(0) {}

void ButtonHandler::init()
{
    pinMode(pin_, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(pin_), ButtonHandler::isr, FALLING);
}

void IRAM_ATTR ButtonHandler::isr()
{
    pressedFlag_ = true;
}

bool ButtonHandler::pollPressed()
{
    if (!pressedFlag_)
    {
        return false;
    }

    uint32_t now = millis();
    if (now - lastDebounceTime_ <= debounceMs_)
    {
        pressedFlag_ = false; // ignore button "ding-dong"
        return false;
    }

    lastDebounceTime_ = now;
    pressedFlag_ = false;
    return true;
}
