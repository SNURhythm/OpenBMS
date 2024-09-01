//
// Created by XF on 8/31/2024.
//

#pragma once
#include <cstdint>
class Color {
public:
  Color() = default;
  Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
  Color(uint8_t r, uint8_t g, uint8_t b);
  explicit Color(uint32_t argb);
  [[nodiscard]] uint32_t toABGR() const;
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;
  uint8_t a = 255;
};
