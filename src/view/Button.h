//
// Created by XF on 8/25/2024.
//

#pragma once
#include "View.h"
#include <functional>
#include <string>
class Button : public View {
private:
  std::function<void()> onClickListener;
  View *contentView;

public:
  Button(int x, int y, int width, int height);
  ~Button() override;
  void render(RenderContext &context) override;
  void onLayout() override;

  void handleEvents(SDL_Event &event) override;
  void setOnClickListener(std::function<void()> listener);
  void setContentView(View *view);
};
