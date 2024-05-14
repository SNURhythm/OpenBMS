#include "Layout.h"
enum class Orientation { HORIZONTAL, VERTICAL };
struct LinearLayoutConfig {
  int width, height;
  float weight;
};
class LinearLayout : public Layout {
public:
  inline LinearLayout(SDL_Renderer *renderer, int x, int y, int width,
                      int height, Orientation orientation)
      : Layout(renderer, x, y, width, height), orientation(orientation) {}
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

private:
  Orientation orientation;
  std::map<View *, LinearLayoutConfig> layoutConfigs;
};
