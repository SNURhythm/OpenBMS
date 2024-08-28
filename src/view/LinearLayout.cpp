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
    auto preferredWidth = std::min(config.width > 0 ? config.width : getWidth(), getWidth() - padding.left - padding.right);
    auto preferredHeight = std::min(config.height > 0 ? config.height : getHeight(), getHeight() - padding.top - padding.bottom);
    if (config.weight > 0) {
      int size = (int)((float)remainingSpace * config.weight / totalWeight);
      if (orientation == Orientation::HORIZONTAL) {
        currentX += config.marginStart;
        auto alignedY = currentY;
        if (align == LinearLayoutAlign::CENTER) {
          alignedY += (getHeight() - preferredHeight) / 2 - padding.top;
        } else if (align == LinearLayoutAlign::END) {
          alignedY += getHeight() - preferredHeight - padding.top;
        }
        view->setPosition(currentX, alignedY);
        view->setSize(size, preferredHeight);
        currentX += size + config.marginEnd + gap;
      } else {
        auto alignedX = currentX;
        if (align == LinearLayoutAlign::CENTER) {
          alignedX += (getWidth() - preferredWidth) / 2 - padding.left;
        } else if (align == LinearLayoutAlign::END) {
          alignedX += getWidth() - preferredWidth - padding.left;
        }
        currentY += config.marginStart;
        view->setPosition(alignedX, currentY);
        view->setSize(preferredWidth, size);
        currentY += size + config.marginEnd + gap;
      }
    } else {
      if (orientation == Orientation::HORIZONTAL) {
        auto alignedY = currentY;
        if (align == LinearLayoutAlign::CENTER) {
          alignedY += (getHeight() - preferredHeight) / 2 - padding.top;
        } else if (align == LinearLayoutAlign::END) {
          alignedY += getHeight() - preferredHeight - padding.top;
        }
        currentX += config.marginStart;
        view->setPosition(currentX, alignedY);
        view->setSize(config.width, preferredHeight);
        currentX += config.width + config.marginEnd + gap;
      } else {
        auto alignedX = currentX;
        if (align == LinearLayoutAlign::CENTER) {
          alignedX += (getWidth() - preferredWidth) / 2 - padding.left;
        } else if (align == LinearLayoutAlign::END) {
          alignedX += getWidth() - preferredWidth - padding.left;
        }
        currentY += config.marginStart;
        view->setPosition(alignedX, currentY);
        view->setSize(preferredWidth, config.height);
        currentY += config.height + config.marginEnd + gap;
      }
    }
    view->onLayout();
  }
}

void LinearLayout::setAlign(LinearLayoutAlign align) {
  this->align = align;
  layout();
}

void LinearLayout::setJustify(LinearLayoutJustify justify) {
  this->justify = justify;
  layout();
}

void LinearLayout::setGap(int gap) {
  this->gap = gap;
  layout();
}