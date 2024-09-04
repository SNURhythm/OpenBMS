//
// Created by XF on 8/25/2024.
//

#pragma once
#include "../Scene.h"
#include "../../bms_parser.hpp"
#include "../../input/IRhythmControl.h"
#include "../../view/TextView.h"
class RhythmInputHandler;
class BMSRenderer;
class GamePlayScene : public Scene, public IRhythmControl {
private:
  bms_parser::Chart *chart;

public:
  GamePlayScene() = delete;
  explicit GamePlayScene(ApplicationContext &context, bms_parser::Chart *chart)
      : Scene(context) {
    this->chart = chart;
  };
  void init() override;
  void update(float dt) override;
  void renderScene() override;
  void cleanupScene() override;
  int pressLane(int lane, double inputDelay) override;
  int pressLane(int mainLane, int compensateLane, double inputDelay) override;
  void releaseLane(int lane, double inputDelay) override;

private:
  BMSRenderer *renderer;
  RhythmInputHandler *inputHandler;
  std::map<int, bool> lanePressed;
  TextView *laneStateText;
};
