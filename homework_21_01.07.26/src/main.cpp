#include <Arduino.h>
#include "App.h"

#define BAUDRATE 115200

App app;

void setup()
{
  Serial.begin(BAUDRATE);
  app.setup();
}

void loop()
{
  app.loop();
}
