//
// Created by XF on 8/31/2024.
//

#include "NoteObject.h"
#include "bgfx/bgfx.h"
#include "bx/math.h"
#include "../../rendering/common.h"
#include "../../rendering/ShaderManager.h"
void NoteObject::render(RenderContext &context) {
  // Draw a 2D rectangle in 3D space
  bgfx::TransientVertexBuffer tvb{};
  bgfx::TransientIndexBuffer tib{};

  // Define the vertex layout
  bgfx::VertexLayout layout = rendering::PosColorVertex::ms_decl;

  bgfx::allocTransientVertexBuffer(&tvb, 4, layout);
  bgfx::allocTransientIndexBuffer(&tib, 6);

  auto *vertices = (rendering::PosColorVertex *)tvb.data;
  auto *index = (uint16_t *)tib.data;

  // Define the width and height of the rectangle
  float width = 1.0f;
  float height = 0.5f;

  // Define the corners of the rectangle in local space (2D in X-Y plane)
  uint32_t abgr = color.toABGR();
  vertices[0] = {0.0f, 0.0f, 0.0f, abgr};
  vertices[1] = {width, 0.0f, 0.0f, abgr};
  vertices[2] = {width, height, 0.0f, abgr};
  vertices[3] = {0.0f, height, 0.0f, abgr};

  // Set up indices for two triangles (quad)
  index[0] = 0;
  index[1] = 1;
  index[2] = 2;
  index[3] = 2;
  index[4] = 3;
  index[5] = 0;

  applyTransform();

  // Set up state (e.g., render state, texture, shaders)
  uint64_t state =
      BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_BLEND_ALPHA;
  bgfx::setState(state);

  // Set the vertex and index buffers
  bgfx::setVertexBuffer(0, &tvb);
  bgfx::setIndexBuffer(&tib);

  // Submit the draw call
  bgfx::submit(
      rendering::main_view,
      rendering::ShaderManager::getInstance().getProgram(SHADER_SIMPLE));
}

void NoteObject::update(float dt) {}
