//
// Created by XF on 9/2/2024.
//

#pragma once

#include "../view/View.h"
#include "GameObject.h"
#include "bgfx/bgfx.h"
class SpriteObject : public GameObject {
public:
  ~SpriteObject() override {
    bgfx::destroy(s_texColor);
    bgfx::destroy(texture);
  }
  float width = 0.1f;
  float height = 0.1f;
  float tileU = 1.0f;
  float tileV = 1.0f;
  explicit SpriteObject(bgfx::TextureHandle texture) : texture(texture) {
    s_texColor = bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);
  }
  void setTexture(bgfx::TextureHandle texture);
  void update(float dt) override;

private:
  void renderImpl(RenderContext &context) override;
  bgfx::UniformHandle s_texColor = BGFX_INVALID_HANDLE;
  bgfx::TextureHandle texture = BGFX_INVALID_HANDLE;
};
