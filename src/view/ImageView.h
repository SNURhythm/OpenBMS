//
// Created by XF on 8/28/2024.
//

#pragma once
#include "View.h"
#include "../path.h"
#include <bgfx/bgfx.h>
#include <map>

class ImageView : public View {
private:
  void renderImpl(RenderContext &context) override;
  struct ImageCache {
    int width, height;
    unsigned char *data;
  };
  void freeTexture();
  bool loadTexture(const path_t &path);
  bgfx::TextureHandle texture = BGFX_INVALID_HANDLE;
  bgfx::UniformHandle s_texColor = BGFX_INVALID_HANDLE;
  static std::map<std::string, ImageCache> imageCache;

public:
  ImageView() = delete;
  ImageView(int x, int y, int width, int height);
  ImageView(int x, int y, int width, int height, const path_t &path);
  ~ImageView() override;
  bool setImage(const path_t &path);
  void freeImage();

  static void dropCache(const path_t &path);
  static void dropAllCache();
};
