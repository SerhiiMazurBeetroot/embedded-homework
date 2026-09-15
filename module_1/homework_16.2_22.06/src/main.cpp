#include <Arduino.h>

#define LDR_PIN 4
#define LED_PIN 5
#define ADC_BITS 12

#define PWM_CHANNEL 0
#define PWM_FREQ 5000
#define PWM_RES 8 // 0-255

int lastPWM = 0;

int readLightAverage()
{
  long sum = 0;
  const int samples = 10;

  for (int i = 0; i < samples; i++)
  {
    sum += analogRead(LDR_PIN);
    delay(2);
  }

  return sum / samples;
}

void setup()
{
  Serial.begin(115200);

  // ADC
  analogReadResolution(ADC_BITS);

  // PWM
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RES);
  ledcAttachPin(LED_PIN, PWM_CHANNEL);
}

void loop()
{
  int adc = readLightAverage(); // 0-4095

  int targetPWM = map(adc, 0, 4095, 255, 0);

  // soft transition
  lastPWM = (lastPWM * 0.85) + (targetPWM * 0.15);

  ledcWrite(PWM_CHANNEL, lastPWM);

  Serial.print("ADC ");
  Serial.print(adc);
  Serial.print(" PWM: ");
  Serial.println(lastPWM);

  delay(20);
}
