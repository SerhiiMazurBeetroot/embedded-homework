#include <Arduino.h>

// GPIO8 -> 100 Om --|--> 100 nF -> GND
//                   |
//                  BTN
//                   |
//                  GND

#define BAUDRATE 115200
#define BTN_PIN 8
#define TASK_NUMBER 1

/*
 * TASKS:
 * 1. Basic implementation (without debounce)
 * 2. Software debounce through a timer (time-based)
 * 3. Debounce via level checking (state-based)
 * 4. Polling + debounce (without interrupts)
 * 5. Hardware debounce
 */

// common variables
volatile uint32_t counter = 0;
volatile bool eventTriggered = false;

// variables task 2, 3
volatile unsigned long lastInterruptTime = 0;
const unsigned long DEBOUNCE_MS = 50;

// variables task 4
enum ButtonState
{
  RELEASED,
  DEBOUNCE,
  PRESSED
};
ButtonState currentState = RELEASED;
unsigned long lastStateChange = 0;

// ISR
void IRAM_ATTR handleInterrupt()
{
#if TASK_NUMBER == 1
  counter++;
  eventTriggered = true;

#elif TASK_NUMBER == 2 || TASK_NUMBER == 3
  eventTriggered = true;

#endif
}

void setup()
{
  Serial.begin(BAUDRATE);
  pinMode(BTN_PIN, INPUT_PULLUP);

#if TASK_NUMBER != 4
  // FALLING: HIGHT -> LOW (press PULLUP)
  attachInterrupt(digitalPinToInterrupt(BTN_PIN), handleInterrupt, FALLING);
#endif

  Serial.printf("Start task %d\n", TASK_NUMBER);
}

void loop()
{
#if TASK_NUMBER == 1
  if (eventTriggered)
  {
    Serial.printf("Task 1: Interrupt triggered! Counter: %u\n", counter);
    eventTriggered = false;
  }

#elif TASK_NUMBER == 2
  if (eventTriggered)
  {
    unsigned long now = millis();
    // ignore if past less 50ms
    if (now - lastInterruptTime >= DEBOUNCE_MS)
    {
      counter++;
      Serial.printf("Task 2: Time-based Debounce. Counter: %u\n", counter);
      lastInterruptTime = now;
    }
    eventTriggered = false;
  }

#elif TASK_NUMBER == 3
  if (eventTriggered)
  {
    // wait level check INPUT_PULLUP = LOW
    if (digitalRead(BTN_PIN) == LOW)
    {
      counter++;
      Serial.printf("Task 3: State-based Debounce. Counter: %u\n", counter);
    }
    eventTriggered = false;
  }

#elif TASK_NUMBER == 4
  int rawReading = digitalRead(BTN_PIN);

  switch (currentState)
  {
  case RELEASED:
    if (rawReading == LOW) // pressed the button
    {
      currentState = DEBOUNCE;
      lastStateChange = millis();
    }
    break;

  case DEBOUNCE:
    if (millis() - lastStateChange >= 10)
    {
      if (digitalRead(BTN_PIN) == LOW)
      {
        currentState = PRESSED;
        counter++;
        Serial.printf("Task 4: Polling Debounce. Counter: %u\n", counter);
      }
      else
      {
        currentState = RELEASED;
      }
    }
    break;

  case PRESSED:
    if (rawReading == HIGH) // button released
    {
      currentState = RELEASED;
    }
    break;
  }
  delay(5);

#endif
}
