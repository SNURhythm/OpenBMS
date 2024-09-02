//
// Created by XF on 9/2/2024.
//

#pragma once

#include <queue>
#include "../../view/View.h"
#include "../../bms_parser.hpp"
#include "../../game/GameObject.h"
#include "../../game/SpriteObject.h"
#include "../../rendering/Color.h"
enum ObjectType {
  Note,
};
class BMSRendererState {
public:
  ~BMSRendererState();
  std::map<bms_parser::Note *, SpriteObject *> noteObjectMap;
  std::map<ObjectType, std::queue<GameObject *>> objectPool;
  size_t currentTimelineIndex = 0;
};
class BMSRenderer {
private:
  int noteHeight = 0;
  int noteWidth = 0;
  std::vector<bms_parser::TimeLine *> timelines;
  BMSRendererState state;
  GameObject *getInstance(ObjectType type);
  void recycleInstance(ObjectType type, GameObject *object);
  void drawLine(RenderContext &context, float height, float y, Color color);
  bgfx::TextureHandle noteTexture = BGFX_INVALID_HANDLE;

public:
  explicit BMSRenderer(bms_parser::Chart *chart);
  void render(RenderContext &context, long long micro);
};
