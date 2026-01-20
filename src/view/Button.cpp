//
// Created by XF on 8/25/2024.
//

#include "Button.h"

void Button::renderImpl(RenderContext &context) {
  context.scissor.x = getX();
  context.scissor.y = getY();
  context.scissor.width = getWidth();
  context.scissor.height = getHeight();

  if (contentView) {
    contentView->render(context);
  }
}

void Button::setOnClickListener(std::function<void()> listener) {
  this->onClickListener = listener;
}

void Button::setContentView(View *view) {
  this->contentView = view;
  view->setPosition(getX(), getY());
  view->setSize(getWidth(), getHeight());
}
Button::~Button() { delete contentView; }
void Button::onLayout() {
  if (contentView) {
    contentView->setPosition(getX(), getY());
    contentView->setSize(getWidth(), getHeight());
  }
}

bool Button::handleEventsImpl(SDL_Event &event) {
  if (contentView) {
    contentView->handleEvents(event);
  }
  // handle click event
  if (event.type == SDL_MOUSEBUTTONDOWN) {
    int mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);
    mouseX = mouseX * rendering::widthScale;
    mouseY = mouseY * rendering::heightScale;
    if (mouseX >= getX() && mouseX <= getX() + getWidth() && mouseY >= getY() &&
        mouseY <= getY() + getHeight()) {
      if (onClickListener) {
        onClickListener();
      }
      return false;
    }
  }
  return true;
}