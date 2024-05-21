#pragma once
#include "View.h"
#include <SDL2/SDL.h>
#include <algorithm>
#include <map>
#include <vector>
struct Padding {
  int left, top, right, bottom;
};
class Layout : public View {
public:
  inline Layout(int x, int y, int width, int height)
      : View(x, y, width, height) {}
  inline ~Layout() {
    for (auto view : views) {
      delete view;
    }
  }
  inline void removeView(View *view) {
    views.erase(std::remove(views.begin(), views.end(), view), views.end());
    layout();
  }
  inline void removeAllViews() {
    views.clear();
    layout();
  }
  void render() override {
    for (auto &view : views) {
      view->render();
    }
  }
  inline void handleEvents(SDL_Event &event) override {
    for (auto &view : views) {
      view->handleEvents(event);
    }
  }

  virtual void layout() = 0;
  inline void setPadding(Padding newPadding) { this->padding = newPadding; }

protected:
  std::vector<View *> views;
  Padding padding = {0, 0, 0, 0};
  inline void onResize(int newWidth, int newHeight) override { layout(); }
  inline void onMove(int newX, int newY) override {
    // SDL_Log("Layout::onMove");
    layout();
  }
};
