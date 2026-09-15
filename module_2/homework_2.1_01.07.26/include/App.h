#pragma once

#include <Arduino.h>
#include "Config.h"
#include "Led.h"
#include "ButtonHandler.h"

class App
{
public:
    void setup();
    void loop();

private:
    // Blinking -> AlwaysOn -> AlwaysOff -> Blinking
    enum class Mode : uint8_t
    {
        Blinking,
        AlwaysOn,
        AlwaysOff
    };

    Led led_{Config::LED_PIN};
    ButtonHandler button_{Config::BUTTON_PIN, Config::DEBOUNCE_MS};

    Mode currentMode_ = {Mode::Blinking};

    uint32_t lastBlinkToggle_ = {0};

    uint32_t iterationCount_{0};
    uint32_t lastReportTime_{0};
    uint64_t totalLoopTimeUs_{0};

    void applyMode();
    void handleButton();
    void handleBlink();
    void reportIterationTime();
};
