//
// Created by XF on 8/30/2024.
//

#pragma once

#include <cmath>
class Vector3;
class Quaternion {
public:
  float x, y, z, w;
  Quaternion() : x(0), y(0), z(0), w(1) {}
  Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
  Quaternion operator+(Quaternion other) const;
  Quaternion operator-(Quaternion other) const;
  Quaternion operator*(Quaternion other) const;
  Quaternion operator*(float scalar) const;
  Quaternion operator*(Vector3 other) const;
  Quaternion operator/(float scalar) const;
  Quaternion &operator+=(Quaternion &other);
  Quaternion &operator-=(Quaternion &other);
  Quaternion &operator*=(Quaternion &other);
  Quaternion &operator*=(float scalar);
  Quaternion &operator/=(float scalar);
  bool operator==(Quaternion &other) const;
  bool operator!=(Quaternion &other) const;
  [[nodiscard]] float dot(const Quaternion &other) const;
  [[nodiscard]] float magnitude() const;
  [[nodiscard]] Quaternion normalized() const;
  [[nodiscard]] Quaternion conjugate() const;
  [[nodiscard]] Quaternion inverse() const;
  [[nodiscard]] Quaternion lerp(const Quaternion &other, float t) const;
  [[nodiscard]] Quaternion slerp(const Quaternion &other, float t) const;
  [[nodiscard]] Vector3 rotate(const Vector3 &v) const;
  [[nodiscard]] Vector3 getEulerAngles() const;
  [[nodiscard]] Vector3 getForward() const;
  [[nodiscard]] Vector3 getUp() const;
  [[nodiscard]] Vector3 getRight() const;
  [[nodiscard]] Vector3 getAxis() const;
  [[nodiscard]] float getAngle() const;
  [[nodiscard]] Vector3 getAxisAngle() const;
  [[nodiscard]] Vector3 toEuler() const;
  void toMatrix(float *matrix) const;

  static Quaternion lookRotation(const Vector3 &forward, const Vector3 &up);
  static Quaternion axisAngle(const Vector3 &axis, float angle);
  static Quaternion fromEuler(float x, float y, float z);
};