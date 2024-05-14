#include "LinearLayout.h"

void LinearLayout::layout() {
  SDL_Log("LinearLayout::layout");
  float totalWeight = 0;
  int totalFixed = 0;
  for (auto &view : views) {
    auto config = layoutConfigs[view];
    if (config.weight > 0) {
      totalWeight += config.weight;
    } else {
      totalFixed +=
          orientation == Orientation::HORIZONTAL ? config.width : config.height;
    }
  }

  int remainingSpace =
      orientation == Orientation::HORIZONTAL
          ? getWidth() - totalFixed - padding.left - padding.right
          : getHeight() - totalFixed - padding.top - padding.bottom;
  int currentX = getX() + padding.left;
  int currentY = getY() + padding.top;
  for (auto &view : views) {
    auto config = layoutConfigs[view];
    if (config.weight > 0) {
      int size = remainingSpace * config.weight / totalWeight;
      if (orientation == Orientation::HORIZONTAL) {
        view->setPosition(currentX, currentY);
        view->setSize(size, getHeight());
        currentX += size;
      } else {
        view->setPosition(currentX, currentY);
        view->setSize(getWidth(), size);
        currentY += size;
      }
    } else {
      if (orientation == Orientation::HORIZONTAL) {
        view->setPosition(currentX, currentY);
        view->setSize(config.width, getHeight());
        currentX += config.width;
      } else {
        view->setPosition(currentX, currentY);
        view->setSize(getWidth(), config.height);
        currentY += config.height;
      }
    }
  }
}