#pragma once

#include <SDL2/SDL.h>

#pragma once

#include <SDL2/SDL.h>

class View {
public:
  inline View(SDL_Renderer *renderer, int x, int y, int width, int height)
      : renderer(renderer), x(x), y(y), width(width), height(height),
        isVisible(true) {}

  virtual ~View() = default;

  virtual void render() = 0;

  inline void handleEvents(SDL_Event &event) {}

  inline void setSize(int newWidth, int newHeight) {
    bool isResized = width != newWidth || height != newHeight;

    width = newWidth;
    height = newHeight;
    if (isResized)
      onResize(newWidth, newHeight);
  }

  inline void setVisible(bool visible) { isVisible = visible; }
  inline bool getVisible() { return isVisible; }
  inline void setPosition(int x, int y) {
    this->x = x;
    this->y = y;
    onMove(x, y);
  }
  inline int getX() { return x; }
  inline int getY() { return y; }
  inline int getWidth() { return width; }
  inline int getHeight() { return height; }

  virtual void onSelected() {}
  virtual void onUnselected() {}

protected:
  SDL_Renderer *renderer;
  // onResize
  virtual void onResize(int newWidth, int newHeight) {}
  // onMove
  virtual void onMove(int newX, int newY) { SDL_Log("View::onMove"); }

private:
  int x, y;          // Position of the view
  int width, height; // Size of the view
  bool isVisible;    // Visibility of the view
};