#include "rgb.h"
#include <SoftPWM.h>

void rgbBegin() {
  for (uint8_t i = 0; i < 3; i++) {
    SoftPWMSet(RGB_PINS[i], 0);
    SoftPWMSetFadeTime(RGB_PINS[i], 100, 100);
  }
}

void rgbWrite(uint32_t color) {
  uint8_t r = (color >> 16) & 0xFF;
  uint8_t g = (color >>  8) & 0xFF;
  uint8_t b = (color >>  0) & 0xFF;
  rgbWrite(r, g, b);
}

void rgbWrite(uint8_t r, uint8_t g, uint8_t b) {
  #ifdef COMMON_ANODE
    r = 255 - r;
    g = 255 - g;
    b = 255 - b;
  #endif
  // Serial.print("rgbWrite(");
  // Serial.print(r);
  // Serial.print(", ");
  // Serial.print(g);
  // Serial.print(", ");
  // Serial.print(b);
  // Serial.println(")");
  SoftPWMSet(RGB_PINS[0], r);
  SoftPWMSet(RGB_PINS[1], g);
  SoftPWMSet(RGB_PINS[2], b);
}

void rgbOff() {
  rgbWrite(0, 0, 0);
}