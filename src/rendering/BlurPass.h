#pragma once

#include <bgfx/bgfx.h>
#include <cstdint>

#include "PostProcessPipeline.h"

namespace rendering {
class BlurPass final : public PostProcessPass {
public:
  BlurPass(uint16_t downsampleFactor, float tintAlpha);
  void init(uint16_t windowW, uint16_t windowH) override;
  void resize(uint16_t windowW, uint16_t windowH) override;
  void execute() override;
  void shutdown() override;

  void setDownsample(uint16_t downsampleFactor);
  void setTintAlpha(float alpha);

  uint16_t sceneWidth() const { return scene_width_; }
  uint16_t sceneHeight() const { return scene_height_; }

private:
  void createFrameBuffers();
  void destroyFrameBuffers();
  void blurHorizontal();
  void blurVertical();
  void drawFinal();

  uint16_t downsample_ = 2;
  float tint_alpha_ = 0.6f;
  uint16_t window_width_ = 1;
  uint16_t window_height_ = 1;
  uint16_t scene_width_ = 1;
  uint16_t scene_height_ = 1;
  bool initialized_ = false;

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
