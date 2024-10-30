#include <SDL2/SDL.h>
#include "bx/allocator.h"
#include "bx/file.h"

#include "common.h"
namespace rendering {
float near_clip = 0.1f;
float far_clip = 100.0f;
void createRect(bgfx::TransientVertexBuffer &tvb,
                bgfx::TransientIndexBuffer &tib, int x, int y, int width,
                int height, uint32_t color) {
  PosColorVertex vertices[] = {
      {float(x), float(y), 0.0f, color},
      {float(x + width), float(y), 0.0f, color},
      {float(x + width), float(y + height), 0.0f, color},
      {float(x), float(y + height), 0.0f, color},
  };

  const uint16_t indices[] = {0, 1, 2, 0, 2, 3};

  bgfx::allocTransientVertexBuffer(&tvb, 4, PosColorVertex::ms_decl);
  bgfx::allocTransientIndexBuffer(&tib, 6);

  bx::memCopy(tvb.data, vertices, sizeof(vertices));
  bx::memCopy(tib.data, indices, sizeof(indices));
}

bgfx::TextureHandle sdlSurfaceToBgfxTexture(SDL_Surface *surface) {
  // Calculate the total size needed for the copy considering pitch
  uint32_t totalSize = surface->h * surface->w * sizeof(Uint32);
  const bgfx::Memory *mem = bgfx::alloc(totalSize);

  // Copy row by row considering the pitch
  uint8_t *dst = (uint8_t *)mem->data;
  uint8_t *src = (uint8_t *)surface->pixels;
  for (int i = 0; i < surface->h; ++i) {
    bx::memCopy(dst, src, surface->w * sizeof(Uint32));
    src += surface->pitch;
    dst += surface->w * sizeof(Uint32);
  }

  return bgfx::createTexture2D((uint16_t)surface->w, (uint16_t)surface->h,
                               false, 1, bgfx::TextureFormat::BGRA8, 0, mem);
}
} // namespace rendering