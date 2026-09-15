#include <Arduino.h>

#define LDR_PIN 4
#define V_REF 3300.0
#define ADC_BITS 12
#define ADC_MAX ((1 << ADC_BITS) - 1)

int MEASURE_PERIOD = 100;

void setup()
{
  Serial.begin(115200);
  delay(1000);

  analogReadResolution(ADC_BITS);
  analogSetPinAttenuation(LDR_PIN, ADC_11db); // range ~0…3.1V

  Serial.println();
  Serial.println(F("=== LDR + ESP32-S3: Light Measurement ==="));
  Serial.printf("Resolution : %d bits (ADC_MAX = %d)\n", ADC_BITS, ADC_MAX);
  Serial.printf("Reference  : %.0f mV\n", V_REF);
  Serial.println("Attenuation: ADC_11db");
  Serial.printf("Period     : %d ms\n", MEASURE_PERIOD);

  Serial.println();
  Serial.println("---------------------------------------------------------------------");
  Serial.println("| No. |  RAW | Vcalc (mV) | Vmeas (mV) | Error (%) |");
  Serial.println("---------------------------------------------------------------------");
}

void loop()
{
  static uint32_t sampleNo = 0;
  sampleNo++;

  // average 16 samples to reduce noise
  uint32_t sum = 0;

  for (uint8_t i = 0; i < 16; i++)
  {
    sum += analogRead(LDR_PIN);
  }

  int rawADC = sum / 16;

  // Calculate voltage based on raw ADC value
  float vCalc = (float)rawADC * V_REF / ADC_MAX;

  // ESP32 calibrated measurement
  int vMeas = analogReadMilliVolts(LDR_PIN);

  // Calculate the percentage error between calculated and measured voltage
  float errorPct = 0.0f;

  if (vMeas > 0)
  {
    errorPct = fabsf(vCalc - (float)vMeas) / (float)vMeas * 100.0f;
  }

  Serial.printf(
      "| %3lu | %4d | %10.2f | %10d | %9.2f |\n",
      sampleNo,
      rawADC,
      vCalc,
      vMeas,
      errorPct);

  delay(MEASURE_PERIOD);
}
