#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#define RGB_PIN 48
#define NUMPIXELS 1

int speedDelay = 150;

Adafruit_NeoPixel pixel(NUMPIXELS, RGB_PIN, NEO_GRB + NEO_KHZ800);

void blink(uint8_t red, uint8_t green, uint8_t blue);

void setup() {
  pixel.begin();
  pixel.clear();
  pixel.show();
}

void loop() {
  blink(255, 0, 0);
  blink(0, 0, 255);
}

void blink(uint8_t red, uint8_t green, uint8_t blue) {
  for (int i = 0; i < 2; i++) {
    pixel.setPixelColor(0, pixel.Color(red, green, blue));
    pixel.show();
    delay(100);

    pixel.clear();
    pixel.show();
    delay(100);
  }

  delay(speedDelay);
}
