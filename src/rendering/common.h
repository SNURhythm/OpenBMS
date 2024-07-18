#pragma once

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

  static void init() {
    ms_decl.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
        .end();
  }
};

struct PosTexVertex {
  float x, y, z;
  float u, v;

  static void init() {
    ms_decl.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .end();
  }

  static bgfx::VertexLayout ms_decl;
};

static bgfx::ViewId ui_view = 255;
static bgfx::ViewId bga_view = 128;
static bgfx::ViewId main_view = 0;

extern int window_width;
extern int window_height;

bgfx::TextureHandle sdlSurfaceToBgfxTexture(SDL_Surface *surface);

void createRect(bgfx::TransientVertexBuffer &tvb,
                bgfx::TransientIndexBuffer &tib, int x, int y, int width,
                int height, uint32_t color);
} // namespace rendering
