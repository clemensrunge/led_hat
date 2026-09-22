#pragma once
#include "Arduino.h"
#include <vector>
#include <algorithm>

typedef uint16_t neoPixelType;
#define NEO_GRB ((1 << 6) | (1 << 4) | (0 << 2) | (2))
#define NEO_RGB ((0 << 6) | (0 << 4) | (1 << 2) | (2))
#define NEO_KHZ800 0x0000

// Colors are kept as 0x00RRGGBB; wire order (NEO_GRB etc.) doesn't matter here.
// show() hands the buffer to the simulator's display.
class Adafruit_NeoPixel
{
public:
  Adafruit_NeoPixel(uint16_t n, int16_t pin = 6, neoPixelType type = NEO_GRB)
      : pixels_(n, 0) {}

  void begin() {}
  void show();
  void setPixelColor(uint16_t n, uint32_t c)
  {
    if (n < pixels_.size()) pixels_[n] = c & 0xFFFFFF;  // out of range is ignored, like the real library
  }
  void setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b) { setPixelColor(n, Color(r, g, b)); }
  uint32_t getPixelColor(uint16_t n) const { return n < pixels_.size() ? pixels_[n] : 0; }
  void clear() { std::fill(pixels_.begin(), pixels_.end(), 0); }
  uint16_t numPixels() const { return pixels_.size(); }

  static uint32_t Color(uint8_t r, uint8_t g, uint8_t b)
  {
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
  }

private:
  std::vector<uint32_t> pixels_;
};
