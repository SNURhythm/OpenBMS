#pragma once
#include "Layout.h"
enum class Orientation { HORIZONTAL, VERTICAL };
struct LinearLayoutConfig {
  int width, height;
  float weight;
  int marginStart, marginEnd;
};
class LinearLayout : public Layout {
public:
  enum LinearLayoutAlign { START, CENTER, END };
  enum LinearLayoutJustify { J_START, J_CENTER, J_END, J_SPACE_BETWEEN, J_SPACE_AROUND }; // TODO: implement
  inline LinearLayout(int x, int y, int width, int height,
                      Orientation orientation)
      : Layout(x, y, width, height), orientation(orientation) {}
  inline void addView(View *view, LinearLayoutConfig layoutConfig) {
    views.push_back(view);
    layoutConfigs[view] = layoutConfig;
    layout();
  }
  void layout() override;

  inline void setOrientation(Orientation newOrientation) {
    this->orientation = newOrientation;
    layout();
  }

  void setAlign(LinearLayoutAlign align);
  void setJustify(LinearLayoutJustify justify);
  void setGap(int gap);
private:
  int gap = 0;
  Orientation orientation;
  std::map<View *, LinearLayoutConfig> layoutConfigs;
  LinearLayoutAlign align = LinearLayoutAlign::START;
  LinearLayoutJustify justify = LinearLayoutJustify::J_START;
};
