#include "App.h"

App::App()
    : blinkers_{
          Blinker(Config::LEDS[0].pin, Config::LEDS[0].intervalMs),
          Blinker(Config::LEDS[1].pin, Config::LEDS[1].intervalMs),
          Blinker(Config::LEDS[2].pin, Config::LEDS[2].intervalMs),
      }
{
}

void App::setup()
{
    for (size_t i = 0; i < Config::LED_COUNT; i++)
    {
        blinkers_[i].init();
    }
}

void App::loop(unsigned long currentMs)
{
    for (size_t i = 0; i < Config::LED_COUNT; i++)
    {
        blinkers_[i].update(currentMs);
    }
}
