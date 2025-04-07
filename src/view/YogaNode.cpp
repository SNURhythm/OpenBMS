#include "YogaNode.h"
#include "View.h"

YogaNode::YogaNode(View *view) : view(view) { node = YGNodeNew(); }

YogaNode::~YogaNode() { YGNodeFree(node); }

YogaNode *YogaNode::setWidth(float width) {
  YGNodeStyleSetWidth(node, width);
  return this;
}

YogaNode *YogaNode::setHeight(float height) {
  YGNodeStyleSetHeight(node, height);
  return this;
}

YogaNode *YogaNode::setFlex(float flex) {
  YGNodeStyleSetFlex(node, flex);
  return this;
}

YogaNode *YogaNode::setFlexGrow(float flexGrow) {
  YGNodeStyleSetFlexGrow(node, flexGrow);
  return this;
}

YogaNode *YogaNode::setFlexWrap(YGWrap flexWrap) {
  YGNodeStyleSetFlexWrap(node, flexWrap);
  return this;
}

YogaNode *YogaNode::setFlexShrink(float flexShrink) {
  YGNodeStyleSetFlexShrink(node, flexShrink);
  return this;
}

YogaNode *YogaNode::setMargin(Edge edge, float margin) {
  YGNodeStyleSetMargin(node, static_cast<YGEdge>(edge), margin);
  return this;
}

YogaNode *YogaNode::setPadding(Edge edge, float padding) {
  YGNodeStyleSetPadding(node, static_cast<YGEdge>(edge), padding);
  return this;
}

YogaNode *YogaNode::setPosition(Edge edge, float position) {
  YGNodeStyleSetPosition(node, static_cast<YGEdge>(edge), position);
  return this;
}

YogaNode *YogaNode::setPositionType(YGPositionType positionType) {
  YGNodeStyleSetPositionType(node, positionType);
  return this;
}

YogaNode *YogaNode::setAlignItems(YGAlign align) {
  YGNodeStyleSetAlignItems(node, align);
  return this;
}

YogaNode *YogaNode::setAlignSelf(YGAlign align) {
  YGNodeStyleSetAlignSelf(node, align);
  return this;
}

YogaNode *YogaNode::setAlignContent(YGAlign align) {
  YGNodeStyleSetAlignContent(node, align);
  return this;
}

YogaNode *YogaNode::setJustifyContent(YGJustify justify) {
  YGNodeStyleSetJustifyContent(node, justify);
  return this;
}

YogaNode *YogaNode::setFlexDirection(FlexDirection direction) {
  YGNodeStyleSetFlexDirection(node, static_cast<YGFlexDirection>(direction));
  return this;
}

YogaNode *YogaNode::setGap(YGGutter gutter, float gap) {
  YGNodeStyleSetGap(node, gutter, gap);
  return this;
}

YogaNode *YogaNode::setGap(float gap) {
  YGNodeStyleSetGap(node, YGGutterAll, gap);
  return this;
}

YogaNode *YogaNode::setDirection(YGDirection direction) {
  YGNodeStyleSetDirection(node, direction);
  return this;
}