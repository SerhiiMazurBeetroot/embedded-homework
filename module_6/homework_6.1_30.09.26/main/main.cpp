#include "app.h"

static App app;


extern "C" void app_main(void)
{
    app.start();
}
