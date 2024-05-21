#pragma once

#include "View.h"
#include "TextView.h"
#include <bgfx/bgfx.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>

class TextInputBox : public TextView {
public:
  TextInputBox(const std::string &fontPath, int fontSize);
  ~TextInputBox() override;

  void handleEvents(SDL_Event &event) override;
  void onSelected() override;
  void onUnselected() override;

  void render() override;

  [[nodiscard]] inline bool getSelected() const { return isSelected; }

private:
  bool isSelected = false;
};
