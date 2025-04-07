#pragma once

#include <SDL2/SDL.h>
#include <yoga/Yoga.h>
#include <vector>
enum class Edge {
  Left = YGEdgeLeft,
  Top = YGEdgeTop,
  Right = YGEdgeRight,
  Bottom = YGEdgeBottom,
  Start = YGEdgeStart,
  End = YGEdgeEnd,
  All = YGEdgeAll
};

enum class FlexDirection {
  Row = YGFlexDirectionRow,
  Column = YGFlexDirectionColumn,
  RowReverse = YGFlexDirectionRowReverse,
  ColumnReverse = YGFlexDirectionColumnReverse
};
struct Scissor {
  int x, y, width, height;
};
struct RenderContext {
  Scissor scissor = {0, 0, -1, -1};
};
class View {
public:
  View() = delete;
  inline View(int x, int y, int width, int height) : isVisible(true) {
    node = YGNodeNew();
    YGNodeStyleSetPosition(node, YGEdgeLeft, x);
    YGNodeStyleSetPosition(node, YGEdgeTop, y);
    YGNodeStyleSetWidth(node, width);
    YGNodeStyleSetHeight(node, height);
    applyYogaLayout();
  }

  virtual ~View() {
    if (node != nullptr) {
      YGNodeFree(node);
      node = nullptr;
    }
    for (auto view : children) {
      delete view;
    }
  }

  void render(RenderContext &context) {
    if (!isVisible)
      return;
    for (auto view : children) {
      view->render(context);
    }
    renderImpl(context);
  }
  void handleEvents(SDL_Event &event) {
    if (!isVisible)
      return;
    handleEventsImpl(event);
  }

  virtual inline void onLayout() {};

  inline void setSize(int newWidth, int newHeight) {
    auto width = YGNodeLayoutGetWidth(node);
    auto height = YGNodeLayoutGetHeight(node);
    bool isResized = width != newWidth || height != newHeight;

    width = newWidth;
    height = newHeight;
    if (isResized) {
      onResize(newWidth, newHeight);
      YGNodeStyleSetWidth(node, width);
      YGNodeStyleSetHeight(node, height);
      applyYogaLayout();
    }
  }

  inline void setVisible(bool visible) { isVisible = visible; }
  [[nodiscard]] inline bool getVisible() const { return isVisible; }
  inline void setPosition(
      int newX, int newY,
      YGPositionType positionType = YGPositionType::YGPositionTypeRelative) {
    YGNodeStyleSetPositionType(node, positionType);
    YGNodeStyleSetPosition(node, YGEdgeLeft, newX);
    YGNodeStyleSetPosition(node, YGEdgeTop, newY);

    applyYogaLayout();
    onMove(newX, newY);
  }
  [[nodiscard]] inline int getX() const { return absoluteX; }
  [[nodiscard]] inline int getY() const { return absoluteY; }
  [[nodiscard]] inline int getWidth() const {
    return YGNodeLayoutGetWidth(node);
  }
  [[nodiscard]] inline int getHeight() const {
    return YGNodeLayoutGetHeight(node);
  }

  virtual void onSelected() {}
  virtual void onUnselected() {}

  View *setWidth(float width);
  View *setHeight(float height);
  View *setFlex(float flex);
  View *setFlexGrow(float flexGrow);
  View *setFlexWrap(YGWrap flexWrap);
  View *setFlexShrink(float flexShrink);
  View *setMargin(Edge edge, float margin);
  View *setPadding(Edge edge, float padding);
  View *setPosition(Edge edge, float position);
  View *setPositionType(YGPositionType positionType);
  View *setAlignItems(YGAlign align);
  View *setAlignSelf(YGAlign align);
  View *setAlignContent(YGAlign align);
  View *setJustifyContent(YGJustify justify);
  View *setFlexDirection(FlexDirection direction);
  View *setGap(YGGutter gutter, float gap);
  View *setGap(float gap);
  View *setDirection(YGDirection direction);
  View *addView(View *view);
  YGNodeRef getNode() const { return node; }
  std::vector<View *> &getChildren() { return children; }

protected:
  virtual void renderImpl(RenderContext &context) {};
  virtual inline void handleEventsImpl(SDL_Event &event) {};
  // onResize
  virtual void onResize(int newWidth, int newHeight) {}
  // onMove
  virtual void onMove(int newX, int newY) {}

private:
  int absoluteX;
  int absoluteY;
  bool isVisible; // Visibility of the view
  YGNodeRef node;

  std::vector<View *> children;

  void applyYogaLayout();
};