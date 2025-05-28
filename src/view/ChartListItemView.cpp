#include "ChartListItemView.h"

ChartListItemView::ChartListItemView(int x, int y, int width, int height,
                                     const bms_parser::ChartMeta &meta)
    : View(x, y, width, height) {
  rootLayout = new YogaLayout(x, y, width, height);
  textLayout = new YogaLayout(x, y, width, height);
  bannerImage = new ImageView(x, y, width, height);
  titleView = new TextView("assets/fonts/notosanscjkjp.ttf", 32);
  titleView->setVAlign(TextView::TextVAlign::BOTTOM);
  artistView = new TextView("assets/fonts/notosanscjkjp.ttf", 16);
  artistView->setVAlign(TextView::TextVAlign::TOP);

  // Configure root layout
  rootLayout->setFlexDirection(YGFlexDirectionRow);
  rootLayout->setAlignItems(YGAlignStretch);

  // Configure text layout
  textLayout->setFlexDirection(YGFlexDirectionColumn);

  // Create and configure nodes for views
  YGNodeRef bannerNode = YGNodeNew();
  YGNodeStyleSetWidth(bannerNode, static_cast<float>(height) / 80.0f * 300.0f);
  YGNodeStyleSetHeight(bannerNode, static_cast<float>(height));
  YGNodeStyleSetMargin(bannerNode, YGEdgeEnd, 8);

  YGNodeRef textLayoutNode = YGNodeNew();
  YGNodeStyleSetFlex(textLayoutNode, 1);

  YGNodeRef titleNode = YGNodeNew();
  YGNodeStyleSetFlexGrow(titleNode, 1.5f);

  YGNodeRef artistNode = YGNodeNew();
  YGNodeStyleSetFlexGrow(artistNode, 1);

  YGNodeRef levelNode = YGNodeNew();
  YGNodeStyleSetWidth(levelNode, 100);
  YGNodeStyleSetHeight(levelNode, 20);

  // Add views with their nodes
  rootLayout->addView(bannerImage, bannerNode);
  rootLayout->addView(textLayout, textLayoutNode);

  textLayout->addView(titleView, titleNode);
  textLayout->addView(artistView, artistNode);

  levelView = new TextView("assets/fonts/notosanscjkjp.ttf", 16);
  levelView->setAlign(TextView::TextAlign::RIGHT);
  levelView->setVAlign(TextView::TextVAlign::MIDDLE);
  rootLayout->addView(levelView, levelNode);

  keyModeOverlay = new TextView("assets/fonts/notosanscjkjp.ttf", 16);
  keyModeOverlay->setColor({255, 0, 0, 255});
  keyModeOverlay->setAlign(TextView::TextAlign::LEFT);
  keyModeOverlay->setVAlign(TextView::TextVAlign::MIDDLE);
}

void ChartListItemView::setMeta(const bms_parser::ChartMeta &meta) {
  std::string title = meta.Title;
  if (!meta.SubTitle.empty()) {
    title += " " + meta.SubTitle;
  }
  titleView->setText(title);
  artistView->setText(meta.Artist);
  levelView->setText(std::to_string(meta.PlayLevel));
  std::string keyModeDesc;
  switch (meta.KeyMode) {
  case 5:
    keyModeDesc = "5K";
    break;
  case 7:
    keyModeDesc = "7K";
    break;
  case 10:
    keyModeDesc = "5KDP";
    break;
  case 14:
    keyModeDesc = "7KDP";
    break;
  }
  keyModeOverlay->setText(keyModeDesc);
  if (!meta.Banner.empty())
    bannerImage->setImage(meta.Folder / meta.Banner);
  else
    bannerImage->freeImage();
}

void ChartListItemView::renderImpl(RenderContext &context) {
  rootLayout->render(context);
  keyModeOverlay->render(context);
}

void ChartListItemView::onSelected() {
  titleView->setColor({255, 0, 0, 255});
  artistView->setColor({255, 0, 0, 255});
}

void ChartListItemView::onUnselected() {
  titleView->setColor({255, 255, 255, 255});
  artistView->setColor({255, 255, 255, 255});
}
