#include "YogaLayout.h"

YogaLayout::YogaLayout(int x, int y, int width, int height)
    : Layout(x, y, width, height) {
  rootNode = YGNodeNew();
  YGNodeStyleSetWidth(rootNode, width);
  YGNodeStyleSetHeight(rootNode, height);
}

YogaLayout::~YogaLayout() {
  for (auto &[view, node] : viewNodes) {
    YGNodeFree(node);
  }
  YGNodeFree(rootNode);
}

void YogaLayout::addView(View *view, const YGNodeRef &config) {
  views.push_back(view);
  viewNodes[view] = config;
  YGNodeInsertChild(rootNode, config, YGNodeGetChildCount(rootNode));
  layout();
}

void YogaLayout::setDirection(YGDirection direction) {
  YGNodeStyleSetDirection(rootNode, direction);
  layout();
}

void YogaLayout::setFlexDirection(YGFlexDirection direction) {
  YGNodeStyleSetFlexDirection(rootNode, direction);
  layout();
}

void YogaLayout::setJustifyContent(YGJustify justify) {
  YGNodeStyleSetJustifyContent(rootNode, justify);
  layout();
}

void YogaLayout::setAlignItems(YGAlign align) {
  YGNodeStyleSetAlignItems(rootNode, align);
  layout();
}

void YogaLayout::setAlignContent(YGAlign align) {
  YGNodeStyleSetAlignContent(rootNode, align);
  layout();
}

void YogaLayout::setFlexWrap(YGWrap wrap) {
  YGNodeStyleSetFlexWrap(rootNode, wrap);
  layout();
}

void YogaLayout::setPadding(YGEdge edge, float padding) {
  YGNodeStyleSetPadding(rootNode, edge, padding);
  layout();
}

void YogaLayout::setGap(YGGutter gutter, float gap) {
  YGNodeStyleSetGap(rootNode, gutter, gap);
  layout();
}

void YogaLayout::updateYogaNode() {
  YGNodeStyleSetWidth(rootNode, getWidth());
  YGNodeStyleSetHeight(rootNode, getHeight());
}

void YogaLayout::applyLayout() {
  int baseX = getX();
  int baseY = getY();
  
  for (auto& [view, node] : viewNodes) {
    int absoluteX = baseX + static_cast<int>(YGNodeLayoutGetLeft(node));
    int absoluteY = baseY + static_cast<int>(YGNodeLayoutGetTop(node));
    
    view->setPosition(absoluteX, absoluteY);
    view->setSize(
        static_cast<int>(YGNodeLayoutGetWidth(node)),
        static_cast<int>(YGNodeLayoutGetHeight(node))
    );
    view->onLayout();
    
    SDL_Log("view %p position: absolute(%d,%d) relative(%d,%d)", 
            view,
            absoluteX, absoluteY,
            static_cast<int>(YGNodeLayoutGetLeft(node)),
            static_cast<int>(YGNodeLayoutGetTop(node)));
  }
}

void YogaLayout::layout() {
  updateYogaNode();
  YGNodeCalculateLayout(rootNode, YGNodeStyleGetWidth(rootNode).value,
                        YGNodeStyleGetHeight(rootNode).value, YGDirectionLTR);
  applyLayout();
}

void YogaLayout::onResize(int newWidth, int newHeight) {
  Layout::onResize(newWidth, newHeight);
  layout();
}

void YogaLayout::onMove(int newX, int newY) {
  Layout::onMove(newX, newY);
  layout();
}