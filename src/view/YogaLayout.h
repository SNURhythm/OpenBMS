#pragma once

#include "Layout.h"
#include <yoga/Yoga.h>
#include <map>

class YogaLayout : public Layout {
public:
    YogaLayout(int x, int y, int width, int height);
    ~YogaLayout() override;

    void addView(View* view, const YGNodeRef& config);
    void setDirection(YGDirection direction);
    void setFlexDirection(YGFlexDirection direction);
    void setJustifyContent(YGJustify justify);
    void setAlignItems(YGAlign align);
    void setAlignContent(YGAlign align);
    void setFlexWrap(YGWrap wrap);
    void setPadding(YGEdge edge, float padding);
    void setGap(YGGutter gutter, float gap);
    
    void layout() override;

protected:
    void onResize(int newWidth, int newHeight) override;
    void onMove(int newX, int newY) override;

private:
    YGNodeRef rootNode;
    std::map<View*, YGNodeRef> viewNodes;
    void updateYogaNode();
    void applyLayout();
}; 