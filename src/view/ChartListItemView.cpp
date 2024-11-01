#include "ChartListItemView.h"

ChartListItemView::ChartListItemView(int x, int y, int width, int height,
                                     const std::string &title,
                                     const std::string &artist,
                                     const std::string &level)
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
  titleView->setText(title);
  artistView->setText(artist);
  textLayout->addView(titleView, titleConfig);
  textLayout->addView(artistView, artistConfig);
  levelView = new TextView("assets/fonts/notosanscjkjp.ttf", 16);
  levelView->setText(level);
  levelView->setAlign(TextView::TextAlign::RIGHT);
  levelView->setVAlign(TextView::TextVAlign::MIDDLE);
  rootLayout->addView(
      bannerImage, {static_cast<int>(height / 80.0 * 300.0), height, 0, 0, 8});
  rootLayout->addView(textLayout, {0, 0, 1});
  rootLayout->addView(levelView, {100, 20, 0});
  rootLayout->setPadding({0, 0, 0, 0});
}

void ChartListItemView::setTitle(const std::string &title) {
  titleView->setText(title);
}

void ChartListItemView::setArtist(const std::string &artist) {
  artistView->setText(artist);
}

void ChartListItemView::setLevel(const std::string &level) {
  levelView->setText(level);
}

void ChartListItemView::setBanner(const path_t &path) {
  bannerImage->setImage(path);
}

void ChartListItemView::unsetBanner() { bannerImage->freeImage(); }

void ChartListItemView::render(RenderContext &context) {
  // SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  // SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128);
  // SDL_Rect rect = {getX(), getY(), getWidth(), getHeight()};
  // SDL_RenderFillRect(renderer, &rect);
  rootLayout->render(context);
}

void ChartListItemView::onSelected() {
  titleView->setColor({255, 0, 0, 255});
  artistView->setColor({255, 0, 0, 255});
}

void ChartListItemView::onUnselected() {
  titleView->setColor({255, 255, 255, 255});
  artistView->setColor({255, 255, 255, 255});
}
