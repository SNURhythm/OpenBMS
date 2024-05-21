#pragma once
#include "bx/allocator.h"
#include "bx/file.h"
#include <bgfx/bgfx.h>
#include <string>
#define SHADER_SIMPLE "simple"
#define SHADER_TEXT "text"
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
static int window_width = 800;
static int window_height = 600;

} // namespace rendering
