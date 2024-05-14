#pragma once
#include "LinearLayout.h"
#include "TextView.h"
#include "View.h"
#include <SDL2/SDL.h>
#include <string>

class ChartListItemView : public View {
public:
  ChartListItemView(SDL_Renderer *renderer, int x, int y, int width, int height,
                    const std::string &title, const std::string &artist,
                    const std::string &level);
  void render() override;
  void setTitle(const std::string &title);
  void setArtist(const std::string &artist);
  void setLevel(const std::string &level);
  void onSelected() override;
  void onUnselected() override;

private:
  LinearLayout *textLayout;
  LinearLayout *rootLayout;
  TextView *titleView;
  TextView *artistView;
  TextView *levelView;

protected:
  inline void onMove(int newX, int newY) override {
    rootLayout->setPosition(newX, newY);
  }
  inline void onResize(int newWidth, int newHeight) override {
    rootLayout->setSize(newWidth, newHeight);
  }
};
