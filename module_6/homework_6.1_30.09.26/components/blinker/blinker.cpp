#include <stdio.h>
#include "blinker.h"
#include "config.h"

bool Blinker::start(const char *name, BaseType_t core, UBaseType_t priority)
{
    return xTaskCreatePinnedToCore(taskEntry, name, Config::TASK_STACK_BYTES,
                                   this, priority, nullptr, core) == pdPASS;
}

void Blinker::taskEntry(void *self)
{
    static_cast<Blinker *>(self)->run();
}

void Blinker::run()
{
    TickType_t lastWake = xTaskGetTickCount();
    for (;;)
    {
        led_.toggle();
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(interval_));
    }
}
