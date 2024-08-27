#pragma once
#include "LinearLayout.h"
#include "TextView.h"
#include "View.h"
#include "ImageView.h"
#include <SDL2/SDL.h>
#include <string>

class ChartListItemView : public View {
public:
  ChartListItemView(int x, int y, int width, int height,
                    const std::string &title, const std::string &artist,
                    const std::string &level);
  void render(RenderContext &context) override;
  void setTitle(const std::string &title);
  void setArtist(const std::string &artist);
  void setLevel(const std::string &level);
  void setBanner(const path_t& path);
  void unsetBanner();
  void onSelected() override;
  void onUnselected() override;

private:
  LinearLayout *textLayout;
  LinearLayout *rootLayout;
  TextView *titleView;
  TextView *artistView;
  TextView *levelView;
  ImageView *bannerImage;

protected:
  inline void onMove(int newX, int newY) override {
    rootLayout->setPosition(newX, newY);
  }
  inline void onResize(int newWidth, int newHeight) override {
    rootLayout->setSize(newWidth, newHeight);
  }
};
