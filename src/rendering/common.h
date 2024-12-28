#pragma once
class Camera;
#include <SDL2/SDL.h>
#include <bgfx/bgfx.h>
#include <string>
#define SHADER_SIMPLE "vs_simple.bin", "fs_simple.bin"
#define SHADER_TEXT "vs_text.bin", "fs_text.bin"
#define SHADER_YUVRGB "vs_yuvrgb.bin", "fs_yuvrgb.bin"
namespace rendering {
struct PosTexCoord0Vertex {
  static bgfx::VertexLayout ms_decl;
  float x, y, z;
  float u, v;
  static void init() {
    ms_decl.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .end();
  }
};
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
static bgfx::ViewId final_view = 5;
static bgfx::ViewId ui_view = 254;
static bgfx::ViewId bga_view = 1;
static bgfx::ViewId bga_layer_view = 2;
static bgfx::ViewId main_view = 128;
static bgfx::ViewId blur_view_h = 3;
static bgfx::ViewId blur_view_v = 4;
static bgfx::ViewId clear_view = 0;
extern Camera *main_camera;
extern Camera game_camera;
extern int window_width;
extern int window_height;
extern float near_clip;
extern float far_clip;

bgfx::TextureHandle sdlSurfaceToBgfxTexture(SDL_Surface *surface);

void createRect(bgfx::TransientVertexBuffer &tvb,
                bgfx::TransientIndexBuffer &tib, int x, int y, int width,
                int height, uint32_t color);

static PosTexCoord0Vertex s_quadVertices[] = {
    {-1.0f, 1.0f, 0.0f, 0.0f, 0.0f},
    {1.0f, 1.0f, 0.0f, 1.0f, 0.0f},
    {-1.0f, -1.0f, 0.0f, 0.0f, 1.0f},
    {1.0f, -1.0f, 0.0f, 1.0f, 1.0f},
};
static const uint16_t s_quadIndices[] = {0, 1, 2, 1, 3, 2};
inline void screenSpaceQuad() {
  bgfx::TransientVertexBuffer tvb{};
  bgfx::TransientIndexBuffer tib{};

  //  SDL_Log("Rendering video texture frame %d; time: %f", currentFrame,
  //  currentFrame / 30.0f);

  bgfx::VertexLayout &layout = rendering::PosTexCoord0Vertex::ms_decl;
  bgfx::allocTransientVertexBuffer(&tvb, 4, layout);
  bgfx::allocTransientIndexBuffer(&tib, 6);
  auto *vertex = (rendering::PosTexCoord0Vertex *)tvb.data;

  // Define quad vertices
  vertex[0].x = -1.0f;
  vertex[0].y = -1.0f;
  vertex[0].z = 0.0f;
  vertex[0].u = 0.0f;
  vertex[0].v = 1.0f;

  vertex[1].x = 1.0f;
  vertex[1].y = -1.0f;
  vertex[1].z = 0.0f;
  vertex[1].u = 1.0f;
  vertex[1].v = 1.0f;

  vertex[2].x = -1.0f;
  vertex[2].y = 1.0f;
  vertex[2].z = 0.0f;
  vertex[2].u = 0.0f;
  vertex[2].v = 0.0f;

  vertex[3].x = 1.0f;
  vertex[3].y = 1.0f;
  vertex[3].z = 0.0f;
  vertex[3].u = 1.0f;
  vertex[3].v = 0.0f;
  auto *indices = (uint16_t *)tib.data;
  indices[0] = 0;
  indices[1] = 1;
  indices[2] = 2;
  indices[3] = 1;
  indices[4] = 3;
  indices[5] = 2;
  bgfx::setVertexBuffer(0, &tvb);
  bgfx::setIndexBuffer(&tib);
}
} // namespace rendering
