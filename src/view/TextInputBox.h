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
  void onMove(int newX, int newY) override;
  void onResize(int newWidth, int newHeight) override;
  void render(RenderContext &context) override;

  [[nodiscard]] inline bool getSelected() const { return isSelected; }

private:
  bool isSelected = false;
  SDL_Rect viewRect;
  size_t cursorPos = 0;

  // convert cursor position to x, y position
  void cursorToPos(size_t cursorPos, int &x, int &y);

  // convert x, y position to cursor position
  size_t posToCursor(int x, int y);

  size_t getNextUnicodePos(size_t pos);
  size_t getPrevUnicodePos(size_t pos);
};
