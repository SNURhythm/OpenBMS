#pragma once

#include "View.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>

class TextView : public View {
public:
  enum TextAlign { LEFT, CENTER, RIGHT };
  enum TextVAlign { TOP, MIDDLE, BOTTOM };
  TextView(SDL_Renderer *renderer, const std::string &fontPath, int fontSize);
  ~TextView();

  void setText(const std::string &text);
  void setColor(SDL_Color color);
  void setAlign(TextAlign align);
  void setVAlign(TextVAlign valign);
  void render() override;

private:
  TextAlign align = TextAlign::LEFT;
  TextVAlign valign = TextVAlign::TOP;
  TTF_Font *font;
  SDL_Texture *texture;
  SDL_Color color;
  SDL_Rect rect;
  std::string text;

  void createTexture();
};
