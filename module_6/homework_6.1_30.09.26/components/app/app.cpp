#include <stdio.h>
#include "app.h"

App::App()
    : blinkers_{
          Blinker(Config::LEDS[0].pin, Config::LEDS[0].intervalMs),
          Blinker(Config::LEDS[1].pin, Config::LEDS[1].intervalMs),
          Blinker(Config::LEDS[2].pin, Config::LEDS[2].intervalMs),
      }
{
}

void App::start()
{
    for (size_t i = 0; i < Config::LED_COUNT; i++)
    {
        blinkers_[i].init();
        blinkers_[i].start(Config::LEDS[i].name, Config::TASK_CORE);
    }
}
