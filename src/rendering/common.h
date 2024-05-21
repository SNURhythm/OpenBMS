#pragma once
#include <bgfx/bgfx.h>
namespace rendering {
struct PosColorVertex {
  static bgfx::VertexLayout ms_decl;
  float x;
  float y;
  float z;
  uint32_t abgr;
};

static bgfx::ViewId ui_view = 255;
static bgfx::ViewId main_view = 0;
// common shader program
static bgfx::ProgramHandle simple_program;
static int window_width = 800;
static int window_height = 600;
} // namespace rendering
