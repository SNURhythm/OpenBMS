//
// Created by XF on 8/25/2024.
//

#pragma once
#include "../Scene.h"
#include "../../bms_parser.hpp"
class GamePlayScene : public Scene {
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
};
