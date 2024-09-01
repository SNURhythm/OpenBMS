//
// Created by XF on 8/25/2024.
//

#include "GamePlayScene.h"
#include "../../view/TextView.h"
void GamePlayScene::init() {
  auto chartNameText = new TextView("assets/fonts/notosanscjkjp.ttf", 32);
  chartNameText->setText("Selected: " + chart->Meta.Title);
  chartNameText->setPosition(10, 10);
  addView(chartNameText);
  testNote.color = Color(0x550000FF);
  testNote.transform.position = {0.0f, 0.0f, 0.0f};
  testNote.transform.rotation = Quaternion::fromEuler(0.0f, 0.0f, 0.0f);
}
void GamePlayScene::update(float dt) {

  Quaternion targetRotation =
      testNote.transform.rotation * Quaternion::fromEuler(1.0f, 1.0f, 1.0f);

  float interpolationFactor = dt * 30;
  testNote.transform.rotation =
      testNote.transform.rotation.slerp(targetRotation, interpolationFactor);

  testNote.transform.position.x += 0.1f * dt;
}

void GamePlayScene::renderScene() {
  RenderContext context;
  testNote.render(context);
}
void GamePlayScene::cleanupScene() {}
