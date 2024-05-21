#pragma once

#include "View.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>

class TextView : public View {
public:
  enum TextAlign { LEFT, CENTER, RIGHT };
  enum TextVAlign { TOP, MIDDLE, BOTTOM };
  TextView(const std::string &fontPath, int fontSize);
  ~TextView() override;

  void setText(const std::string &newText);
  void setColor(SDL_Color newColor);
  void setAlign(TextAlign newAlign);
  void setVAlign(TextVAlign newVAlign);
  void render() override;

private:
  TextAlign align = TextAlign::LEFT;
  TextVAlign valign = TextVAlign::TOP;
  TTF_Font *font;
  SDL_Texture *texture;
  SDL_Color color{};
  SDL_Rect rect{};
  std::string text;

  void createTexture();
};
