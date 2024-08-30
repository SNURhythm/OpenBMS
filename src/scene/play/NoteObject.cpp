//
// Created by XF on 8/31/2024.
//

#include "NoteObject.h"
#include "bgfx/bgfx.h"
void NoteObject::render(RenderContext &context) {
  // draw 2D rectangle in 3D space
  bgfx::TransientVertexBuffer tvb{};
  bgfx::TransientIndexBuffer tib{};

  bgfx::VertexLayout layout{};
  layout.begin().add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float).end();

  bgfx::allocTransientVertexBuffer(&tvb, 4, layout);
  bgfx::allocTransientIndexBuffer(&tib, 6);

  auto *vertex = (float *)tvb.data;
  auto *index = (uint16_t *)tib.data;

  // 2D rectangle
  auto &position = this->transform.position;
  vertex[0] = position.x;
  vertex[1] = position.y;
  vertex[2] = position.z;
  vertex[3] = position.x + 1;
  vertex[4] = position.y;
  vertex[5] = position.z;
  vertex[6] = position.x + 1;
  vertex[7] = position.y + 1;
  vertex[8] = position.z;
  vertex[9] = position.x;
  vertex[10] = position.y + 1;
  vertex[11] = position.z;
}
void NoteObject::update(float dt) {}
