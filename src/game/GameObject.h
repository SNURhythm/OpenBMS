//
// Created by XF on 8/30/2024.
//

#pragma once
#include "../math/Vector3.h"
#include "../math/Quaternion.h"
struct Transform {
  Vector3 position;
  Quaternion rotation;
  Vector3 scale;
};
class GameObject {
public:
  Transform transform;
  GameObject() : transform({Vector3(0, 0, 0), Quaternion(0, 0, 0, 1), Vector3(1, 1, 1)}) {}
  GameObject(Vector3 position, Quaternion rotation, Vector3 scale) : transform({position, rotation, scale}) {}
  void translate(Vector3 translation) { transform.position += translation; }
  void rotate(Quaternion rotation) { transform.rotation *= rotation; }
  void scale(Vector3 scale) { transform.scale *= scale; }
  void setTransform(Transform transform) { this->transform = transform; }
  Transform getTransform() { return transform; }
  Vector3 getPosition() { return transform.position; }
  Quaternion getRotation() { return transform.rotation; }
  Vector3 getScale() { return transform.scale; }
  Vector3 getForward() { return transform.rotation.getForward(); }
  Vector3 getUp() { return transform.rotation.getUp(); }
  Vector3 getRight() { return transform.rotation.getRight(); }
  Vector3 getAxis() { return transform.rotation.getAxis(); }
  float getAngle() { return transform.rotation.getAngle(); }
  Vector3 getAxisAngle() { return transform.rotation.getAxisAngle(); }
  void setPosition(Vector3 position) { transform.position = position; }
  void setRotation(Quaternion rotation) { transform.rotation = rotation; }
  void setScale(Vector3 scale) { transform.scale = scale; }
  void setForward(Vector3 forward) { transform.rotation = Quaternion::lookRotation(forward, transform.rotation.getUp()); }
  void setUp(Vector3 up) { transform.rotation = Quaternion::lookRotation(transform.rotation.getForward(), up); }
  void setRight(Vector3 right) { transform.rotation = Quaternion::lookRotation(transform.rotation.getForward(), transform.rotation.getUp()); }
  void setAxis(Vector3 axis) { transform.rotation = Quaternion::axisAngle(axis, transform.rotation.getAngle()); }
  void setAngle(float angle) { transform.rotation = Quaternion::axisAngle(transform.rotation.getAxis(), angle); }
  void setAxisAngle(Vector3 axis, float angle) { transform.rotation = Quaternion::axisAngle(axis, angle); }
  void lookAt(Vector3 target) { transform.rotation = Quaternion::lookRotation(target - transform.position, transform.rotation.getUp()); }
  void lookAt(Vector3 target, Vector3 up) { transform.rotation = Quaternion::lookRotation(target - transform.position,
};
