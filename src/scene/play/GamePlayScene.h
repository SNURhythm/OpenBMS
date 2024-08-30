//
// Created by XF on 8/25/2024.
//

#pragma once
#include "../Scene.h"
#include "../../bms_parser.hpp"
#include "NoteObject.h"
class GamePlayScene : public Scene {
private:
  bms_parser::Chart *chart;

public:
  NoteObject testNote;
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
