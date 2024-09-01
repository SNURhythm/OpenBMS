//
// Created by XF on 8/31/2024.
//

#include "Color.h"
Color::Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
    : r(r), g(g), b(b), a(a) {}
Color::Color(uint8_t r, uint8_t g, uint8_t b) : r(r), g(g), b(b), a(255) {}
Color::Color(uint32_t argb) {
  a = (argb >> 24) & 0xFF;
  r = (argb >> 16) & 0xFF;
  g = (argb >> 8) & 0xFF;
  b = argb & 0xFF;
}
uint32_t Color::toABGR() const { return (a << 24) | (b << 16) | (g << 8) | r; }
