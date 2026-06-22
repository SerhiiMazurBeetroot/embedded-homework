#include <Arduino.h>

#define LED_1_PIN 13
#define LED_2_PIN 12
#define BUTTON_EXT 4
#define BUTTON_BOOT 0

#define DEBOUNCE_MS 50

unsigned long blinkDelay = 1000;
bool ledState = LOW;

void setup()
{
  Serial.begin(115200);

  pinMode(LED_1_PIN, OUTPUT);
  pinMode(LED_2_PIN, OUTPUT);
  pinMode(BUTTON_EXT, INPUT_PULLUP);
  pinMode(BUTTON_BOOT, INPUT_PULLUP);

  Serial.println("Setup complete. Current mode: SLOW");
}

void waitRelease(int pin)
{
  delay(DEBOUNCE_MS);
  while (digitalRead(pin) == LOW)
  {
    delay(5);
  }
  delay(DEBOUNCE_MS);
}

void loop()
{
  // external: FAST
  if (digitalRead(BUTTON_EXT) == LOW)
  {
    delay(DEBOUNCE_MS);
    if (digitalRead(BUTTON_EXT) == LOW)
    {
      blinkDelay = 200;
      Serial.println("Mode: FAST");
      waitRelease(BUTTON_EXT);
    }
  }

  // boot: SLOW
  if (digitalRead(BUTTON_BOOT) == LOW)
  {
    delay(DEBOUNCE_MS);
    if (digitalRead(BUTTON_BOOT) == LOW)
    {
      blinkDelay = 1000;
      Serial.println("Mode: SLOW");
      waitRelease(BUTTON_BOOT);
    }
  }

  // blinking
  ledState = !ledState;
  digitalWrite(LED_1_PIN, ledState);
  digitalWrite(LED_2_PIN, ledState);
  delay(blinkDelay);
}
