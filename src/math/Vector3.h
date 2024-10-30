//
// Created by XF on 8/30/2024.
//

#pragma once

class Vector3 {
public:
  float x, y, z;
  Vector3() : x(0.0f), y(0.0f), z(0.0f) {}
  Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

  Vector3 operator+(Vector3 other) const;
  Vector3 operator-(Vector3 other) const;
  Vector3 operator*(float scalar) const;
  Vector3 operator/(float scalar) const;
  Vector3 &operator+=(Vector3 &other);
  Vector3 &operator-=(Vector3 &other);
  Vector3 &operator*=(float scalar);
  Vector3 &operator/=(float scalar);
  Vector3 &operator*=(Vector3 &other);
  bool operator==(Vector3 &other) const;
  bool operator!=(Vector3 &other) const;
  [[nodiscard]] float dot(const Vector3 &other) const;
  [[nodiscard]] Vector3 cross(const Vector3 &other) const;
  [[nodiscard]] float magnitude() const;
  [[nodiscard]] Vector3 normalized() const;
  [[nodiscard]] float distance(const Vector3 &other) const;
  [[nodiscard]] float angle(const Vector3 &other) const;
  [[nodiscard]] Vector3 lerp(const Vector3 &other, float t) const;
  [[nodiscard]] Vector3 slerp(const Vector3 &other, float t) const;
  [[nodiscard]] Vector3 project(const Vector3 &other) const;
  [[nodiscard]] Vector3 reflect(const Vector3 &normal) const;
  [[nodiscard]] Vector3 refract(const Vector3 &normal, float eta) const;
  [[nodiscard]] Vector3 rotate(const Vector3 &axis, float angle) const;
};