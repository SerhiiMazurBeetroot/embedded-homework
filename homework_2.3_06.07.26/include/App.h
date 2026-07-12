#pragma once
#include <Arduino.h>
#include "Config.h"
#include "Blinker.h"

class App
{
public:
    App();

    void setup();
    void loop(unsigned long currentMs);

private:
    Blinker blinkers_[Config::LED_COUNT];
};
