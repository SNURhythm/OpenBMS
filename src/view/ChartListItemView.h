#pragma once
#include "YogaLayout.h"
#include "TextView.h"
#include "View.h"
#include "ImageView.h"
#include <SDL2/SDL.h>
#include <string>
#include "../bms_parser.hpp"

class ChartListItemView : public View {
public:
  ChartListItemView(int x, int y, int width, int height,
                    const bms_parser::ChartMeta &meta);
  ~ChartListItemView() override{
    delete rootLayout;
    delete keyModeOverlay;
  }

  void setMeta(const bms_parser::ChartMeta &meta);
  void onSelected() override;
  void onUnselected() override;

private:
  void renderImpl(RenderContext &context) override;
  YogaLayout *textLayout;
  YogaLayout *rootLayout;
  TextView *titleView;
  TextView *artistView;
  TextView *levelView;
  ImageView *bannerImage;
  TextView *keyModeOverlay;

protected:
  inline void onMove(int newX, int newY) override {
    rootLayout->setPosition(newX, newY);
    keyModeOverlay->setPosition(newX, newY);
  }
  inline void onResize(int newWidth, int newHeight) override {
    rootLayout->setSize(newWidth, newHeight);
    keyModeOverlay->setSize(newWidth, newHeight);
  }
};
