//
// Created by XF on 8/31/2024.
//

#pragma once

#include "../../game/GameObject.h"
#include "../../rendering/Color.h"
class NoteObject : public GameObject {
public:
  void update(float dt) override;
  void render(RenderContext &context) override;
  Color color;
};
