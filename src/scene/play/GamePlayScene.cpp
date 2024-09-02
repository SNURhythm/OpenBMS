//
// Created by XF on 8/25/2024.
//

#include "GamePlayScene.h"
#include "../../view/TextView.h"
#include "BMSRenderer.h"
void GamePlayScene::init() {
  auto chartNameText = new TextView("assets/fonts/notosanscjkjp.ttf", 32);
  chartNameText->setText("Selected: " + chart->Meta.Title);
  chartNameText->setPosition(10, 10);
  addView(chartNameText);
  renderer = new BMSRenderer(chart);
  context.jukebox.seek(0);
  context.jukebox.play();
}
void GamePlayScene::update(float dt) {}

void GamePlayScene::renderScene() {
  RenderContext renderContext;
  renderer->render(renderContext, context.jukebox.getTimeMicros());
}
void GamePlayScene::cleanupScene() {}
