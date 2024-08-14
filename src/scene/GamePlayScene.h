//
// Created by XF on 8/25/2024.
//

#pragma once
#include "Scene.h"
#include "../bms_parser.hpp"
class GamePlayScene : public Scene {
private:
  bms_parser::ChartMeta chartMeta;
public:
  GamePlayScene() = delete;
  explicit GamePlayScene(const bms_parser::ChartMeta& chartMeta){
      this->chartMeta = chartMeta;
  };
  void init(ApplicationContext &context) override;
  void update(float dt) override;
  void renderScene() override;
  void cleanupScene() override;


};

