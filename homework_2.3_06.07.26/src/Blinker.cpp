#include "Blinker.h"

void Blinker::update(unsigned long currentMs)
{
    if (currentMs - prevMs_ >= interval_)
    {
        prevMs_ = currentMs;
        led_.toggle();
    }
}
