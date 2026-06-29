#include <Arduino.h>

#define BUTTON_PIN 15

volatile uint32_t counter = 0;

void IRAM_ATTR buttonISR()
{
  counter++;
}

void setup()
{
  Serial.begin(115200);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  attachInterrupt(
      digitalPinToInterrupt(BUTTON_PIN),
      buttonISR,
      CHANGE);
}

void loop()
{
  static uint32_t lastCounter = 0;

  noInterrupts();
  uint32_t current = counter;
  interrupts();

  if (current != lastCounter)
  {
    Serial.println(current);
    lastCounter = current;
  }
}
