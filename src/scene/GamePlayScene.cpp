//
// Created by XF on 8/25/2024.
//

#include "GamePlayScene.h"
#include "../view/TextView.h"
void GamePlayScene::init(ApplicationContext &context) {
  auto chartNameText = new TextView("assets/fonts/notosanscjkjp.ttf", 32);
  chartNameText->setText("Selected: "+chartMeta.Title);
  chartNameText->setPosition(10, 10);
  addView(chartNameText);
}
void GamePlayScene::update(float dt) {}
void GamePlayScene::renderScene() {}
void GamePlayScene::cleanupScene() {}
