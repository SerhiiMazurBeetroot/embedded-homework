#include <Arduino.h>

#define BAUDRATE 115200

#define RELAY_PIN 10
#define NO_PIN 9

const int TOTAL_MEASUREMENTS = 10;

volatile unsigned long startTime = 0;
volatile unsigned long endTime = 0;
volatile bool isMeasured = false;

void IRAM_ATTR handleRelayISR()
{
  if (!isMeasured)
  {
    endTime = millis();
    isMeasured = true;
  }
}

void setup()
{
  Serial.begin(BAUDRATE);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // init state is OFF

  // INPUT_PULLUP, because COM -> GND
  pinMode(NO_PIN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(NO_PIN), handleRelayISR, FALLING);

  Serial.println("\n Start of relay operation ");
  delay(1000);

  unsigned long totalResponseTime = 0;

  for (int i = 1; i <= TOTAL_MEASUREMENTS; i++)
  {
    isMeasured = false;

    // signal to RELAY ON
    startTime = millis();
    digitalWrite(RELAY_PIN, HIGH);

    // wait interrupt
    unsigned long timeoutStart = millis();
    while (!isMeasured && (millis() - timeoutStart < 1000))
    {
      yield(); // esp32 can run other tasks
    }

    if (isMeasured)
    {
      unsigned long duration = endTime - startTime;
      totalResponseTime += duration;

      Serial.printf("Measurement #%d : %lu ms \n", i, duration);
    }
    else
    {
      Serial.printf("Measurement #%d : Error (timeout)\n", i);
    }

    // disable RELAY and wait next test
    digitalWrite(RELAY_PIN, LOW);
    delay(500);
  }

  // calculate avg
  float avgTime = (float)totalResponseTime / TOTAL_MEASUREMENTS;

  Serial.println("----------------------------------------------");
  Serial.printf("AVG relay operating time: %.2f ms\n", avgTime);
  Serial.println("----------------------------------------------");
}

void loop()
{
}
