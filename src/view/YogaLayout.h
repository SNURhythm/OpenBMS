#pragma once

#include "Layout.h"
#include "YogaNode.h"
#include <yoga/Yoga.h>
#include <map>

// Forward declarations
class View;

class YogaLayout : public Layout {
public:
  YogaLayout(int x, int y, int width, int height);
  ~YogaLayout() override;

  YogaNode *addView(View *view);
  YogaNode *getRootNode() { return rootYogaNode; }
  YGNodeRef getYGNode() const { return rootNode; }

  void layout() override;

protected:
  void onResize(int newWidth, int newHeight) override;
  void onMove(int newX, int newY) override;

private:
  YGNodeRef rootNode;
  YogaNode *rootYogaNode;
  std::map<View *, YogaNode *> viewNodes;
  void updateYogaNode();
  void applyLayout();
};