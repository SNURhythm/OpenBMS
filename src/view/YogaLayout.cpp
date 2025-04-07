#include "YogaLayout.h"
#include "View.h"

YogaLayout::YogaLayout(int x, int y, int width, int height)
    : Layout(x, y, width, height) {
  rootNode = YGNodeNew();
  rootYogaNode = new YogaNode(this);
  rootYogaNode->node = rootNode;

  YGNodeStyleSetPosition(rootNode, YGEdgeLeft, x);
  YGNodeStyleSetPosition(rootNode, YGEdgeTop, y);
  YGNodeStyleSetWidth(rootNode, width);
  YGNodeStyleSetHeight(rootNode, height);
}

YogaLayout::~YogaLayout() {
  for (auto &[view, node] : viewNodes) {
    delete node;
  }
  delete rootYogaNode;
}

YogaNode *YogaLayout::addView(View *view) {
  auto node = new YogaNode(view);
  viewNodes[view] = node;
  views.push_back(view);
  YGNodeInsertChild(rootNode, node->getNode(), YGNodeGetChildCount(rootNode));
  layout();
  return node;
}

void YogaLayout::updateYogaNode() {
  YGNodeStyleSetPosition(rootNode, YGEdgeLeft, getX());
  YGNodeStyleSetPosition(rootNode, YGEdgeTop, getY());
  YGNodeStyleSetWidth(rootNode, getWidth());
  YGNodeStyleSetHeight(rootNode, getHeight());
}

void YogaLayout::applyLayout() {
  int baseX = getX();
  int baseY = getY();

  for (auto &[view, node] : viewNodes) {
    auto ygNode = node->getNode();
    int absoluteX = baseX + static_cast<int>(YGNodeLayoutGetLeft(ygNode));
    int absoluteY = baseY + static_cast<int>(YGNodeLayoutGetTop(ygNode));

    view->setPosition(absoluteX, absoluteY);
    view->setSize(static_cast<int>(YGNodeLayoutGetWidth(ygNode)),
                  static_cast<int>(YGNodeLayoutGetHeight(ygNode)));
    view->onLayout();
  }
}

void YogaLayout::layout() {
  updateYogaNode();
  YGNodeCalculateLayout(rootNode, YGUndefined, YGUndefined, YGDirectionLTR);
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