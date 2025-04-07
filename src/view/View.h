#pragma once

#include <SDL2/SDL.h>
#include <yoga/Yoga.h>
#include <vector>
#include "../rendering/common.h"
#include "../rendering/ShaderManager.h"
#include "../rendering/Color.h"
#include "bgfx/bgfx.h"
#include "bgfx/defines.h"
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
  inline View(int x, int y, int width, int height) : isVisible(true) {
    dbgColor = {static_cast<uint8_t>(rand() % 256),
                static_cast<uint8_t>(rand() % 256),
                static_cast<uint8_t>(rand() % 256), 64};
    node = YGNodeNew();
    YGNodeStyleSetPosition(node, YGEdgeLeft, x);
    YGNodeStyleSetPosition(node, YGEdgeTop, y);
    YGNodeStyleSetWidth(node, width);
    YGNodeStyleSetHeight(node, height);
    applyYogaLayout();
  }
  inline View() : isVisible(true) {
    dbgColor = {static_cast<uint8_t>(rand() % 256),
                static_cast<uint8_t>(rand() % 256),
                static_cast<uint8_t>(rand() % 256), 64};
    node = YGNodeNew();
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
    if (drawBoundingBox) {
      float x = getX();
      float y = getY();
      float width = getWidth();
      float height = getHeight();
      bgfx::TransientVertexBuffer tvb{};
      bgfx::TransientIndexBuffer tib{};
      // Define the vertex layout
      bgfx::VertexLayout layout = rendering::PosColorVertex::ms_decl;

      bgfx::allocTransientVertexBuffer(&tvb, 4, layout);
      bgfx::allocTransientIndexBuffer(&tib, 6);

      auto *vertices = (rendering::PosColorVertex *)tvb.data;
      auto *index = (uint16_t *)tib.data;

      uint32_t abgr = dbgColor.toABGR();
      vertices[0] = {x, y, 0.0f, abgr};
      vertices[1] = {x + width, y, 0.0f, abgr};
      vertices[2] = {x + width, y + height, 0.0f, abgr};
      vertices[3] = {x, y + height, 0.0f, abgr};

      // Set up indices for two triangles (quad)
      index[0] = 0;
      index[1] = 1;
      index[2] = 2;
      index[3] = 2;
      index[4] = 3;
      index[5] = 0;

      // Set up state (e.g., render state, texture, shaders)
      uint64_t state = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
                       BGFX_STATE_BLEND_ALPHA | BGFX_STATE_MSAA;
      bgfx::setState(state);

      // Set the vertex and index buffers
      bgfx::setVertexBuffer(0, &tvb);
      bgfx::setIndexBuffer(&tib);

      // Submit the draw call
      bgfx::submit(
          rendering::ui_view,
          rendering::ShaderManager::getInstance().getProgram(SHADER_SIMPLE));
    }
    for (auto view : children) {
      view->render(context);
    }
    renderImpl(context);
  }
  void handleEvents(SDL_Event &event) {
    if (!isVisible)
      return;
    handleEventsImpl(event);
    for (auto view : children) {
      view->handleEvents(event);
    }
  }

  virtual inline void onLayout() {};

  inline void setSize(int newWidth, int newHeight) {
    auto width = YGNodeLayoutGetWidth(node);
    auto height = YGNodeLayoutGetHeight(node);
    bool isResized = width != newWidth || height != newHeight;

    width = newWidth;
    height = newHeight;
    if (isResized) {
      SDL_Log("View::setSize: %d, %d", newWidth, newHeight);
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

  bool drawBoundingBox = false;
  void applyYogaLayout();

protected:
  virtual void renderImpl(RenderContext &context) {};
  virtual inline bool handleEventsImpl(SDL_Event &event) { return true; };
  // onResize
  virtual void onResize(int newWidth, int newHeight) {}
  // onMove
  virtual void onMove(int newX, int newY) {}

private:
  Color dbgColor;
  int absoluteX;
  int absoluteY;
  bool isVisible; // Visibility of the view
  YGNodeRef node;

  std::vector<View *> children;
};