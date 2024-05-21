#include "TextView.h"

TextView::TextView(const std::string &fontPath, int fontSize)
    : View(0, 0, 0, 0), texture(nullptr) {
  TTF_Init();
  font = TTF_OpenFont(fontPath.c_str(), fontSize);
  if (!font) {
    SDL_Log("Failed to load font: %s", TTF_GetError());
  }
  // fall back to default font
  if (!font) {
    font = TTF_OpenFont("assets/fonts/arial.ttf", fontSize);
  }
  color = {255, 255, 255, 255}; // default color: white
  rect = {0, 0, 0, 0};
}

TextView::~TextView() {
  if (texture) {
    SDL_DestroyTexture(texture);
  }
  TTF_CloseFont(font);
  TTF_Quit();
}

void TextView::setText(const std::string &newText) {
  // this->text = newText;
  // SDL_Surface *surface = TTF_RenderText_Blended(font, newText.c_str(),
  // color); if (texture) {
  //   SDL_DestroyTexture(texture);
  // }
  // texture = SDL_CreateTextureFromSurface(renderer, surface);
  // SDL_FreeSurface(surface);

  // SDL_QueryTexture(texture, nullptr, nullptr, &rect.w, &rect.h);
}

void TextView::setColor(SDL_Color newColor) {
  this->color = newColor;
  // Update the texture since newColor has changed
  createTexture();
}

void TextView::render() {
  // rect.x = this->getX();
  // rect.y = this->getY();
  // auto width = this->getWidth();
  // auto height = this->getHeight();
  // switch (align) {
  // case TextAlign::LEFT:
  //   break;
  // case TextAlign::CENTER:
  //   rect.x += (width - rect.w) / 2; // center horizontally
  //   break;
  // case TextAlign::RIGHT:
  //   rect.x += width - rect.w; // align right
  //   break;
  // }
  // switch (valign) {
  // case TextVAlign::TOP:
  //   break;
  // case TextVAlign::MIDDLE:
  //   rect.y += (height - rect.h) / 2; // center vertically
  //   break;
  // case TextVAlign::BOTTOM:
  //   rect.y += height - rect.h; // align bottom
  //   break;
  // }

  // if (texture) {
  //   SDL_RenderCopy(renderer, texture, nullptr, &rect);
  // }
}

void TextView::createTexture() { setText(text); }

void TextView::setAlign(TextAlign newAlign) { this->align = newAlign; }

void TextView::setVAlign(TextVAlign newVAlign) { this->valign = newVAlign; }