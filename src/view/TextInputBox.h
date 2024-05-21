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

  void render(RenderContext &context) override;

  [[nodiscard]] inline bool getSelected() const { return isSelected; }

private:
  bool isSelected = false;
  SDL_Rect viewRect;
  int cursorPos = 0;

  int popBack(std::string &utf8);

  // convert cursor position to x, y position
  void cursorToPos(int cursorPos, int &x, int &y);

  // convert x, y position to cursor position
  int posToCursor(int x, int y);

  int getNextUnicodePos(int pos);
};
