#include "TextView.h"
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <cstring>
#include "../rendering/common.h"
#include "../rendering/ShaderManager.h"
#include "bgfx/defines.h"
#include "bx/math.h"

TextView::TextView(const std::string &fontPath, int fontSize)
    : View(0, 0, 0, 0), texture(BGFX_INVALID_HANDLE) {
  TTF_Init();
  font = TTF_OpenFont(fontPath.c_str(), fontSize);
  if (!font) {
    SDL_Log("Failed to load font: %s", TTF_GetError());
    font = TTF_OpenFont("assets/fonts/arial.ttf", fontSize); // Fallback font
  }
  color = {255, 255, 255, 255}; // Default color: white
  rect = {0, 0, 0, 0};
  s_texColor = bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);
}

TextView::~TextView() {
  if (bgfx::isValid(texture)) {
    bgfx::destroy(texture);
  }
  bgfx::destroy(s_texColor);
  TTF_CloseFont(font);
  TTF_Quit();
}

void TextView::setText(const std::string &newText) {
  this->text = newText;
  if (newText.empty()) {
    rect.w = 0;
    rect.h = 0;
    if (bgfx::isValid(texture)) {
      bgfx::destroy(texture);
    }
    return;
  }
  SDL_Surface *surface = TTF_RenderUTF8_Blended(font, newText.c_str(), color);
  if (bgfx::isValid(texture)) {
    bgfx::destroy(texture);
  }
  if (!surface) {
    SDL_Log("Failed to render text: %s", TTF_GetError());
    return;
  }
  rect.w = surface->w;
  rect.h = surface->h;
  texture = rendering::sdlSurfaceToBgfxTexture(surface);
  SDL_FreeSurface(surface);
}

void TextView::render(RenderContext &context) {
  if (bgfx::isValid(texture)) {

    rect.x = this->getX();
    rect.y = this->getY();
    auto width = this->getWidth();
    auto height = this->getHeight();
    switch (align) {
    case TextAlign::LEFT:
      break;
    case TextAlign::CENTER:
      rect.x += (width - rect.w) / 2; // center horizontally
      break;
    case TextAlign::RIGHT:
      rect.x += width - rect.w; // align right
      break;
    }
    switch (valign) {
    case TextVAlign::TOP:
      break;
    case TextVAlign::MIDDLE:
      rect.y += (height - rect.h) / 2; // center vertically
      break;
    case TextVAlign::BOTTOM:
      rect.y += height - rect.h; // align bottom
      break;
    }

    rendering::PosTexVertex vertices[] = {
        {0.0f, 0.0f, 0.0f, 0.0f, 0.0f},                   // Top-left
        {(float)rect.w, 0.0f, 0.0f, 1.0f, 0.0f},          // Top-right
        {(float)rect.w, (float)rect.h, 0.0f, 1.0f, 1.0f}, // Bottom-right
        {0.0f, (float)rect.h, 0.0f, 0.0f, 1.0f}           // Bottom-left
    };

    const uint16_t indices[] = {0, 1, 2, 0, 2, 3};
    bgfx::TransientVertexBuffer tvb;
    bgfx::TransientIndexBuffer tib;
    bgfx::allocTransientVertexBuffer(&tvb, 4, rendering::PosTexVertex::ms_decl);
    bgfx::allocTransientIndexBuffer(&tib, 6);
    bx::memCopy(tvb.data, vertices, sizeof(vertices));
    bx::memCopy(tib.data, indices, sizeof(indices));

    float translate[16];
    bx::mtxTranslate(translate, rect.x, rect.y, 0.0f);
    bgfx::setTransform(translate);
    bgfx::setTexture(0, s_texColor, texture);
    bgfx::setVertexBuffer(0, &tvb);
    bgfx::setIndexBuffer(&tib);

    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
                   BGFX_STATE_BLEND_ALPHA);
    bgfx::setScissor(context.scissor.x, context.scissor.y,
                     context.scissor.width, context.scissor.height);
    bgfx::submit(
        rendering::ui_view,
        rendering::ShaderManager::getInstance().getProgram(SHADER_TEXT));
  }
}

void TextView::setColor(SDL_Color newColor) {
  this->color = newColor;
  createTexture(); // Update the texture since newColor has changed
}

void TextView::createTexture() {
  if (!text.empty()) {
    setText(text); // Re-create texture with new color
  }
}

void TextView::setAlign(TextAlign newAlign) { this->align = newAlign; }

void TextView::setVAlign(TextVAlign newVAlign) { this->valign = newVAlign; }