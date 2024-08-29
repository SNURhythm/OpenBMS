//
// Created by XF on 8/30/2024.
//

#include "Quaternion.h"
#include "Vector3.h"
Quaternion Quaternion::operator+(Quaternion other) const {
  return {x + other.x, y + other.y, z + other.z, w + other.w};
}
Quaternion Quaternion::operator-(Quaternion other) const {
  return {x - other.x, y - other.y, z - other.z, w - other.w};
}
Quaternion Quaternion::operator*(Quaternion other) const {
  float rX = x * other.w + w * other.x + y * other.z - z * other.y;
  float rY = y * other.w + w * other.y + z * other.x - x * other.z;
  float rZ = z * other.w + w * other.z + x * other.y - y * other.x;
  float rW = w * other.w - x * other.x - y * other.y - z * other.z;
  return {rX, rY, rZ, rW};
}
Quaternion Quaternion::operator*(float scalar) const {
  return {x * scalar, y * scalar, z * scalar, w * scalar};
}
Quaternion Quaternion::operator/(float scalar) const {
  return {x / scalar, y / scalar, z / scalar, w / scalar};
}
Quaternion &Quaternion::operator+=(Quaternion &other) {
  x += other.x;
  y += other.y;
  z += other.z;
  w += other.w;
  return *this;
}
Quaternion &Quaternion::operator-=(Quaternion &other) {
  x -= other.x;
  y -= other.y;
  z -= other.z;
  w -= other.w;
  return *this;
}
Quaternion &Quaternion::operator*=(Quaternion &other) {
  float rX = x * other.w + w * other.x + y * other.z - z * other.y;
  float rY = y * other.w + w * other.y + z * other.x - x * other.z;
  float rZ = z * other.w + w * other.z + x * other.y - y * other.x;
  float rW = w * other.w - x * other.x - y * other.y - z * other.z;
  x = rX;
  y = rY;
  z = rZ;
  w = rW;
  return *this;
}
Quaternion &Quaternion::operator*=(float scalar) {
  x *= scalar;
  y *= scalar;
  z *= scalar;
  w *= scalar;
  return *this;
}
Quaternion &Quaternion::operator/=(float scalar) {
  x /= scalar;
  y /= scalar;
  z /= scalar;
  w /= scalar;
  return *this;
}
bool Quaternion::operator==(Quaternion &other) const {
  return x == other.x && y == other.y && z == other.z && w == other.w;
}
bool Quaternion::operator!=(Quaternion &other) const {
  return x != other.x || y != other.y || z != other.z || w != other.w;
}
float Quaternion::dot(const Quaternion &other) const {
  return x * other.x + y * other.y + z * other.z + w * other.w;
}
float Quaternion::magnitude() const {
  return sqrtf(x * x + y * y + z * z + w * w);
}
Quaternion Quaternion::normalized() const {
  float mag = magnitude();
  return {x / mag, y / mag, z / mag, w / mag};
}
Quaternion Quaternion::conjugate() const { return {-x, -y, -z, w}; }
Quaternion Quaternion::inverse() const {
  return conjugate() / (x * x + y * y + z * z + w * w);
}
Quaternion Quaternion::lerp(const Quaternion &other, float t) const {
  return *this * (1 - t) + other * t;
}
Quaternion Quaternion::slerp(const Quaternion &other, float t) const {
  float dotProduct = dot(other);
  float theta = acosf(dotProduct);
  return (*this * sinf((1 - t) * theta) + other * sinf(t * theta)) /
         sinf(theta);
}
Vector3 Quaternion::rotate(const Vector3 &v) const {
  Quaternion conjugate = this->conjugate();
  Quaternion w = (*this * Quaternion(v.x, v.y, v.z, 0) * conjugate);
  return {w.x, w.y, w.z};
}
Vector3 Quaternion::getEulerAngles() const {
  float x = atan2f(2 * (w * x + y * z), 1 - 2 * (x * x + y * y));
  float y = asinf(2 * (w * y - z * x));
  float z = atan2f(2 * (w * z + x * y), 1 - 2 * (y * y + z * z));
  return {x, y, z};
}
Vector3 Quaternion::getForward() const {
  return {2 * (x * z + w * y), 2 * (y * z - w * x), 1 - 2 * (x * x + y * y)};
}
Vector3 Quaternion::getUp() const {
  return {2 * (x * y - w * z), 1 - 2 * (x * x + z * z), 2 * (y * z + w * x)};
}
Vector3 Quaternion::getRight() const {
  return {1 - 2 * (y * y + z * z), 2 * (x * y + w * z), 2 * (x * z - w * y)};
}
Vector3 Quaternion::getAxis() const { return {x, y, z}; }
float Quaternion::getAngle() const { return 2 * acosf(w); }
Vector3 Quaternion::getAxisAngle() const { float s = sqrtf(1 - w * w); }
