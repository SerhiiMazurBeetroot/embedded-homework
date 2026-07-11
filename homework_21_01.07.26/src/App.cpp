#include "App.h"

void App::setup()
{
    led_.init();
    button_.init();

    lastReportTime_ = millis();
}

void App::loop()
{
    const uint32_t start = micros();

    handleButton();
    handleBlink();

    totalLoopTimeUs_ += micros() - start;

    reportIterationTime();
}

void App::applyMode()
{
    switch (currentMode_)
    {
    case Mode::AlwaysOn:
        led_.set(LedState::On);
        break;

    case Mode::AlwaysOff:
        led_.set(LedState::Off);
        break;

    case Mode::Blinking:
    default:
        // here working handleBlick
        break;
    }
}

void App::handleButton()
{
    if (!button_.pollPressed())
    {
        return;
    }

    switch (currentMode_)
    {
    case Mode::Blinking:
        currentMode_ = Mode::AlwaysOn;
        break;

    case Mode::AlwaysOn:
        currentMode_ = Mode::AlwaysOff;
        break;

    case Mode::AlwaysOff:
    default:
        currentMode_ = Mode::Blinking;
        break;
    }

    applyMode();
}

void App::handleBlink()
{
    if (currentMode_ != Mode::Blinking)
    {
        return;
    }

    uint32_t now = millis();
    if (now - lastBlinkToggle_ >= Config::BLINK_INTERVAL_MS)
    {
        lastBlinkToggle_ = now;
        led_.toggle();
    }
}

void App::reportIterationTime()
{
    ++iterationCount_;

    if (iterationCount_ < Config::ITERATIONS_TO_REPORT)
        return;

    Serial.print("Avg loop: ");
    Serial.print(
        static_cast<float>(totalLoopTimeUs_) /
        Config::ITERATIONS_TO_REPORT);
    Serial.println(" us");

    iterationCount_ = 0;
    totalLoopTimeUs_ = 0;
}
