#pragma once
#include "Layout.h"
#include <yoga/Yoga.h>

class YogaNode {
public:
  YogaNode(View *view);
  ~YogaNode();

  YogaNode *setWidth(float width);
  YogaNode *setHeight(float height);
  YogaNode *setFlex(float flex);
  YogaNode *setFlexGrow(float flexGrow);
  YogaNode *setFlexWrap(YGWrap flexWrap);
  YogaNode *setFlexShrink(float flexShrink);
  YogaNode *setMargin(Edge edge, float margin);
  YogaNode *setPadding(Edge edge, float padding);
  YogaNode *setPosition(Edge edge, float position);
  YogaNode *setPositionType(YGPositionType positionType);
  YogaNode *setAlignItems(YGAlign align);
  YogaNode *setAlignSelf(YGAlign align);
  YogaNode *setAlignContent(YGAlign align);
  YogaNode *setJustifyContent(YGJustify justify);
  YogaNode *setFlexDirection(FlexDirection direction);
  YogaNode *setGap(YGGutter gutter, float gap);
  YogaNode *setGap(float gap);
  YogaNode *setDirection(YGDirection direction);

  [[nodiscard]] View *getView() const { return view; }
  [[nodiscard]] YGNodeRef getNode() const { return node; }

private:
  View *view;
  YGNodeRef node;
  friend class YogaLayout;
};