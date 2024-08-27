#include "LinearLayout.h"

void LinearLayout::layout() {
  float totalWeight = 0;
  int totalFixed = 0;
  for (auto &view : views) {
    auto config = layoutConfigs[view];
    if (config.weight > 0) {
      totalWeight += config.weight;
    } else {
      auto preferredWidth = config.width > 0 ? config.width : view->getWidth();
        auto preferredHeight =
            config.height > 0 ? config.height : view->getHeight();
      totalFixed +=
          orientation == Orientation::HORIZONTAL ? preferredWidth : preferredHeight;
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
      int size = (int)((float)remainingSpace * config.weight / totalWeight);
      if (orientation == Orientation::HORIZONTAL) {
        currentX += config.marginStart;
        view->setPosition(currentX, currentY);
        view->setSize(size, getHeight());
        currentX += size + config.marginEnd;
      } else {
        currentY += config.marginStart;
        view->setPosition(currentX, currentY);
        view->setSize(getWidth(), size);
        currentY += size + config.marginEnd;
      }
    } else {
      if (orientation == Orientation::HORIZONTAL) {
        currentX += config.marginStart;
        view->setPosition(currentX, currentY);
        view->setSize(config.width, getHeight());
        currentX += config.width + config.marginEnd;
      } else {
        currentY += config.marginStart;
        view->setPosition(currentX, currentY);
        view->setSize(getWidth(), config.height);
        currentY += config.height + config.marginEnd;
      }
    }
    view->onLayout();
  }
}