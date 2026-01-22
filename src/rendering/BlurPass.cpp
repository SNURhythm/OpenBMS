#include "BlurPass.h"

#include "ShaderManager.h"
#include "common.h"

namespace rendering {
BlurPass::BlurPass(uint16_t downsampleFactor, float tintAlpha)
    : downsample_(downsampleFactor > 0 ? downsampleFactor : 1),
      tint_alpha_(tintAlpha) {}

void BlurPass::init(uint16_t windowW, uint16_t windowH) {
  window_width_ = windowW;
  window_height_ = windowH;
  prog_blur_h_ = ShaderManager::getInstance().getProgram(
      "blur/vs_blur.bin", "blur/fs_blurH.bin");
  prog_blur_v_ = ShaderManager::getInstance().getProgram(
      "blur/vs_blur.bin", "blur/fs_blurV.bin");
  prog_rect_ = ShaderManager::getInstance().getProgram("blur/vs_blur.bin",
                                                       "fs_rect_tint.bin");

  if (!bgfx::isValid(u_tex_color_)) {
    u_tex_color_ = bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);
  }
  if (!bgfx::isValid(u_texel_size_)) {
    u_texel_size_ =
        bgfx::createUniform("u_texelSize", bgfx::UniformType::Vec4);
  }
  if (!bgfx::isValid(u_tint_color_)) {
    u_tint_color_ =
        bgfx::createUniform("u_tintColor", bgfx::UniformType::Vec4);
  }

  resize(windowW, windowH);
  initialized_ = true;
}

void BlurPass::resize(uint16_t windowW, uint16_t windowH) {
  window_width_ = windowW;
  window_height_ = windowH;
  destroyFrameBuffers();

  scene_width_ = windowW / downsample_;
  scene_height_ = windowH / downsample_;
  if (scene_width_ < 1)
    scene_width_ = 1;
  if (scene_height_ < 1)
    scene_height_ = 1;

  createFrameBuffers();
}

void BlurPass::execute() {
  if (!initialized_)
    return;
  blurHorizontal();
  blurVertical();
  drawFinal();
}

void BlurPass::shutdown() {
  destroyFrameBuffers();

  if (bgfx::isValid(u_tex_color_))
    bgfx::destroy(u_tex_color_);
  if (bgfx::isValid(u_texel_size_))
    bgfx::destroy(u_texel_size_);
  if (bgfx::isValid(u_tint_color_))
    bgfx::destroy(u_tint_color_);

  u_tex_color_ = BGFX_INVALID_HANDLE;
  u_texel_size_ = BGFX_INVALID_HANDLE;
  u_tint_color_ = BGFX_INVALID_HANDLE;

  initialized_ = false;
}

void BlurPass::setDownsample(uint16_t downsampleFactor) {
  downsample_ = downsampleFactor > 0 ? downsampleFactor : 1;
  resize(window_width_, window_height_);
}

void BlurPass::setTintAlpha(float alpha) { tint_alpha_ = alpha; }

void BlurPass::createFrameBuffers() {
  tex_scene_color_ =
      bgfx::createTexture2D(scene_width_, scene_height_, false, 1,
                            bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_RT);
  fb_scene_ = bgfx::createFrameBuffer(1, &tex_scene_color_, true);

  tex_blur_a_ =
      bgfx::createTexture2D(scene_width_, scene_height_, false, 1,
                            bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_RT);
  fb_blur_a_ = bgfx::createFrameBuffer(1, &tex_blur_a_, true);

  tex_blur_b_ =
      bgfx::createTexture2D(scene_width_, scene_height_, false, 1,
                            bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_RT);
  fb_blur_b_ = bgfx::createFrameBuffer(1, &tex_blur_b_, true);

  bgfx::setViewFrameBuffer(rendering::bga_view, fb_scene_);
  bgfx::setViewFrameBuffer(rendering::bga_layer_view, fb_scene_);

  bgfx::setViewRect(rendering::blur_view_h, 0, 0, scene_width_, scene_height_);
  bgfx::setViewRect(rendering::blur_view_v, 0, 0, scene_width_, scene_height_);
}

void BlurPass::destroyFrameBuffers() {
  if (bgfx::isValid(fb_scene_))
    bgfx::destroy(fb_scene_);
  if (bgfx::isValid(fb_blur_a_))
    bgfx::destroy(fb_blur_a_);
  if (bgfx::isValid(fb_blur_b_))
    bgfx::destroy(fb_blur_b_);

  if (bgfx::isValid(tex_scene_color_))
    bgfx::destroy(tex_scene_color_);
  if (bgfx::isValid(tex_blur_a_))
    bgfx::destroy(tex_blur_a_);
  if (bgfx::isValid(tex_blur_b_))
    bgfx::destroy(tex_blur_b_);

  fb_scene_ = BGFX_INVALID_HANDLE;
  fb_blur_a_ = BGFX_INVALID_HANDLE;
  fb_blur_b_ = BGFX_INVALID_HANDLE;
  tex_scene_color_ = BGFX_INVALID_HANDLE;
  tex_blur_a_ = BGFX_INVALID_HANDLE;
  tex_blur_b_ = BGFX_INVALID_HANDLE;
}

void BlurPass::blurHorizontal() {
  bgfx::setViewFrameBuffer(rendering::blur_view_h, fb_blur_a_);
  bgfx::setViewRect(rendering::blur_view_h, 0, 0, scene_width_, scene_height_);

  bgfx::setTexture(0, u_tex_color_, tex_scene_color_);
  rendering::screenSpaceQuad();

  float texelSize[4];
  texelSize[0] = 1.0f / float(scene_width_);
  texelSize[1] = 1.0f / float(scene_height_);
  texelSize[2] = 0.0f;
  texelSize[3] = 0.0f;
  bgfx::setUniform(u_texel_size_, texelSize);

  bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
  bgfx::submit(rendering::blur_view_h, prog_blur_h_);
}

void BlurPass::blurVertical() {
  bgfx::setViewFrameBuffer(rendering::blur_view_v, fb_blur_b_);
  bgfx::setViewRect(rendering::blur_view_v, 0, 0, scene_width_, scene_height_);

  bgfx::setTexture(0, u_tex_color_, tex_blur_a_);
  rendering::screenSpaceQuad();

  float texelSize[4];
  texelSize[0] = 1.0f / float(scene_width_);
  texelSize[1] = 1.0f / float(scene_height_);
  texelSize[2] = 0.0f;
  texelSize[3] = 0.0f;
  bgfx::setUniform(u_texel_size_, texelSize);

  bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
  bgfx::submit(rendering::blur_view_v, prog_blur_v_);
}

void BlurPass::drawFinal() {
  bgfx::setViewFrameBuffer(rendering::final_view, BGFX_INVALID_HANDLE);
  bgfx::setViewRect(rendering::final_view, rendering::ui_offset_x,
                    rendering::ui_offset_y, rendering::ui_view_width,
                    rendering::ui_view_height);

  bgfx::setTexture(0, u_tex_color_, tex_blur_b_);
  float tintColor[4] = {1.0f, 1.0f, 1.0f, tint_alpha_};
  bgfx::setUniform(u_tint_color_, tintColor);
  bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
  rendering::screenSpaceQuad();
  bgfx::submit(rendering::final_view, prog_rect_);
}
} // namespace rendering
