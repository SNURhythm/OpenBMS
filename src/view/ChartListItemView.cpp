#include "ChartListItemView.h"

ChartListItemView::ChartListItemView(int x, int y, int width, int height, const bms_parser::ChartMeta &meta
                                     )
    : View(x, y, width, height) {
  rootLayout = new LinearLayout(x, y, width, height, Orientation::HORIZONTAL);
  textLayout = new LinearLayout(x, y, width, height, Orientation::VERTICAL);
  bannerImage = new ImageView(x, y, width, height);
  titleView = new TextView("assets/fonts/notosanscjkjp.ttf", 32);
  titleView->setVAlign(TextView::TextVAlign::BOTTOM);
  artistView = new TextView("assets/fonts/notosanscjkjp.ttf", 16);
  artistView->setVAlign(TextView::TextVAlign::TOP);
  LinearLayoutConfig titleConfig{};

  titleConfig.width = 0;
  titleConfig.height = 0;
  titleConfig.weight = 1.5;
  LinearLayoutConfig artistConfig{};

  artistConfig.width = 0;
  artistConfig.height = 0;
  artistConfig.weight = 1;
  textLayout->addView(titleView, titleConfig);
  textLayout->addView(artistView, artistConfig);
  levelView = new TextView("assets/fonts/notosanscjkjp.ttf", 16);
  levelView->setAlign(TextView::TextAlign::RIGHT);
  levelView->setVAlign(TextView::TextVAlign::MIDDLE);
  rootLayout->addView(
      bannerImage, {static_cast<int>(height / 80.0 * 300.0), height, 0, 0, 8});
  rootLayout->addView(textLayout, {0, 0, 1});
  rootLayout->addView(levelView, {100, 20, 0});
  rootLayout->setPadding({0, 0, 0, 0});

  keyModeOverlay = new TextView("assets/fonts/notosanscjkjp.ttf", 16);
  keyModeOverlay->setColor({255, 0, 0, 255});
  keyModeOverlay->setAlign(TextView::TextAlign::LEFT);
  keyModeOverlay->setVAlign(TextView::TextVAlign::MIDDLE);

}
void ChartListItemView::setMeta(const bms_parser::ChartMeta &meta){
  titleView->setText(meta.Title);
  artistView->setText(meta.Artist);
  levelView->setText(std::to_string(meta.PlayLevel));
  std::string keyModeDesc;
  switch(meta.KeyMode){
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
  if(!meta.Banner.empty())
    bannerImage->setImage(meta.Folder / meta.Banner);
  else
    bannerImage->freeImage();

}

void ChartListItemView::renderImpl(RenderContext &context) {
  // SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  // SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128);
  // SDL_Rect rect = {getX(), getY(), getWidth(), getHeight()};
  // SDL_RenderFillRect(renderer, &rect);
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
