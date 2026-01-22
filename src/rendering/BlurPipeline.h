#pragma once

#include <bgfx/bgfx.h>
#include <cstdint>

namespace rendering {
class BlurPipeline {
public:
  void init(uint16_t windowW, uint16_t windowH, uint16_t downsampleFactor = 2);
  void resize(uint16_t windowW, uint16_t windowH);
  void shutdown();
  void apply();

  uint16_t sceneWidth() const { return scene_width_; }
  uint16_t sceneHeight() const { return scene_height_; }

private:
  void createFrameBuffers();
  void destroyFrameBuffers();
  void blurHorizontal();
  void blurVertical();
  void drawFinal();

  bool initialized_ = false;
  uint16_t downsample_ = 2;
  uint16_t scene_width_ = 1;
  uint16_t scene_height_ = 1;

  bgfx::ProgramHandle prog_blur_h_ = BGFX_INVALID_HANDLE;
  bgfx::ProgramHandle prog_blur_v_ = BGFX_INVALID_HANDLE;
  bgfx::ProgramHandle prog_rect_ = BGFX_INVALID_HANDLE;

  bgfx::UniformHandle u_tex_color_ = BGFX_INVALID_HANDLE;
  bgfx::UniformHandle u_texel_size_ = BGFX_INVALID_HANDLE;
  bgfx::UniformHandle u_tint_color_ = BGFX_INVALID_HANDLE;

  bgfx::TextureHandle tex_scene_color_ = BGFX_INVALID_HANDLE;
  bgfx::FrameBufferHandle fb_scene_ = BGFX_INVALID_HANDLE;

  bgfx::TextureHandle tex_blur_a_ = BGFX_INVALID_HANDLE;
  bgfx::FrameBufferHandle fb_blur_a_ = BGFX_INVALID_HANDLE;

  bgfx::TextureHandle tex_blur_b_ = BGFX_INVALID_HANDLE;
  bgfx::FrameBufferHandle fb_blur_b_ = BGFX_INVALID_HANDLE;
};
} // namespace rendering
