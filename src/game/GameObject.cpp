//
// Created by XF on 8/30/2024.
//

#include "GameObject.h"
#include "bgfx/bgfx.h"
#include <bx/math.h>
GameObject::GameObject()
    : transform({Vector3(0, 0, 0), Quaternion(0, 0, 0, 1), Vector3(1, 1, 1)}) {}
GameObject::GameObject(Vector3 position, Quaternion rotation, Vector3 scale)
    : transform({position, rotation, scale}) {}
GameObject::GameObject(Vector3 position, Quaternion rotation)
    : transform({position, rotation, Vector3(1, 1, 1)}) {}
GameObject::GameObject(Vector3 position)
    : transform({position, Quaternion(0, 0, 0, 1), Vector3(1, 1, 1)}) {}
void GameObject::translate(Vector3 translation) {
  transform.position += translation;
}
void GameObject::rotate(Quaternion rotation) { transform.rotation *= rotation; }
void GameObject::scale(Vector3 scale) { transform.scale *= scale; }
void GameObject::setForward(Vector3 forward) {
  transform.rotation =
      Quaternion::lookRotation(forward, transform.rotation.getUp());
}
void GameObject::setUp(Vector3 up) {
  transform.rotation =
      Quaternion::lookRotation(transform.rotation.getForward(), up);
}
void GameObject::setRight(Vector3 right) {
  transform.rotation = Quaternion::lookRotation(transform.rotation.getForward(),
                                                transform.rotation.getUp());
}
void GameObject::setAxis(Vector3 axis) {
  transform.rotation =
      Quaternion::axisAngle(axis, transform.rotation.getAngle());
}
void GameObject::setAngle(float angle) {
  transform.rotation =
      Quaternion::axisAngle(transform.rotation.getAxis(), angle);
}
void GameObject::setAxisAngle(Vector3 axis, float angle) {
  transform.rotation = Quaternion::axisAngle(axis, angle);
}
void GameObject::lookAt(Vector3 target) {
  transform.rotation = Quaternion::lookRotation(target - transform.position,
                                                transform.rotation.getUp());
}
void GameObject::lookAt(Vector3 target, Vector3 up) {
  transform.rotation =
      Quaternion::lookRotation(target - transform.position, up);
}
void GameObject::applyTransform() const {
  // Initialize an identity matrix for the final transformation matrix
  float transformMatrix[16];
  bx::mtxIdentity(transformMatrix);

  // Convert quaternion to rotation matrix
  float rotationMatrix[16];
  transform.rotation.toMatrix(rotationMatrix);

  // Create scaling matrix
  float scaleMatrix[16];
  bx::mtxScale(scaleMatrix, transform.scale.x, transform.scale.y,
               transform.scale.z);

  // Create translation matrix
  float translationMatrix[16];
  bx::mtxTranslate(translationMatrix, transform.position.x,
                   transform.position.y, transform.position.z);

  // Combine the matrices in the correct order: scale -> rotation -> translation
  float tempMatrix[16];
  bx::mtxMul(tempMatrix, scaleMatrix, rotationMatrix); // Scale, then rotate
  bx::mtxMul(transformMatrix, tempMatrix, translationMatrix); // Then translate

  // Set the transformation matrix in bgfx
  bgfx::setTransform(transformMatrix);
}
