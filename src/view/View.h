#pragma once

#include <SDL2/SDL.h>
struct Scissor {
  int x, y, width, height;
};
struct RenderContext {
  Scissor scissor = {0, 0, -1, -1};
};
class View {
public:
  inline View(int x, int y, int width, int height)
      : x(x), y(y), width(width), height(height), isVisible(true) {}

  virtual ~View() = default;

  virtual void render(RenderContext &context) = 0;

  virtual inline void handleEvents(SDL_Event &event) {}

  inline void setSize(int newWidth, int newHeight) {
    bool isResized = width != newWidth || height != newHeight;

    width = newWidth;
    height = newHeight;
    if (isResized)
      onResize(newWidth, newHeight);
  }

  inline void setVisible(bool visible) { isVisible = visible; }
  [[nodiscard]] inline bool getVisible() const { return isVisible; }
  inline void setPosition(int newX, int newY) {
    this->x = newX;
    this->y = newY;
    onMove(newX, newY);
  }
  [[nodiscard]] inline int getX() const { return x; }
  [[nodiscard]] inline int getY() const { return y; }
  [[nodiscard]] inline int getWidth() const { return width; }
  [[nodiscard]] inline int getHeight() const { return height; }

  virtual void onSelected() {}
  virtual void onUnselected() {}

protected:
  // onResize
  virtual void onResize(int newWidth, int newHeight) {}
  // onMove
  virtual void onMove(int newX, int newY) {}

private:
  int x, y;          // Position of the view
  int width, height; // Size of the view
  bool isVisible;    // Visibility of the view
};