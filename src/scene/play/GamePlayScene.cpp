//
// Created by XF on 8/25/2024.
//

#include "GamePlayScene.h"
#include "../../view/TextView.h"
#include "BMSRenderer.h"
#include "../../input/RhythmInputHandler.h"
void GamePlayScene::init() {
  auto chartNameText = new TextView("assets/fonts/notosanscjkjp.ttf", 32);
  chartNameText->setText("Selected: " + chart->Meta.Title);
  chartNameText->setPosition(10, 10);
  addView(chartNameText);
  renderer = new BMSRenderer(chart);
  context.jukebox.seek(0);
  context.jukebox.play();
  inputHandler = new RhythmInputHandler(this, chart->Meta);
  inputHandler->startListenSDL();
  laneStateText = new TextView("assets/fonts/notosanscjkjp.ttf", 32);
  laneStateText->setPosition(100, 100);
  for (const auto &lane : chart->Meta.GetTotalLaneIndices()) {
    lanePressed[lane] = false;
  }
}
void GamePlayScene::update(float dt) {}

void GamePlayScene::renderScene() {
  RenderContext renderContext;
  renderer->render(renderContext, context.jukebox.getTimeMicros());
  context.jukebox.render();
  std::string str;
  for (auto &[lane, pressed] : lanePressed) {
    str += std::to_string(pressed) + "\n";
  }
  laneStateText->setText(str);
  laneStateText->render(renderContext);
}
void GamePlayScene::cleanupScene() {}
int GamePlayScene::pressLane(int lane, double inputDelay) {
  SDL_Log("press lane: %d, delay: %f", lane, inputDelay);
  lanePressed[lane] = true;
  return lane;
}
int GamePlayScene::pressLane(int mainLane, int compensateLane,
                             double inputDelay) {
  SDL_Log("press lane: %d, %d, delay: %f", mainLane, compensateLane,
          inputDelay);
  lanePressed[mainLane] = true;
  return mainLane;
}
void GamePlayScene::releaseLane(int lane, double inputDelay) {
  SDL_Log("release lane: %d, delay: %f", lane, inputDelay);
  lanePressed[lane] = false;
}
