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
#include "../../scene/play/RhythmState.h"
#include "../../view/TextView.h"

#include <list>
struct LaneState {
  long long lastStateTime = -1;
  bool isPressed = false;
  JudgeResult lastPressedJudge = JudgeResult(None, 0);
};
class JudgeResult;
enum ObjectType {
  Note,
  LongBody,
};
class BMSRendererState {
public:
  ~BMSRendererState();
  std::map<bms_parser::Note *, SpriteObject *> noteObjectMap;
  std::map<bms_parser::LongNote *, SpriteObject *> longBodyObjectMap;
  std::map<ObjectType, std::queue<GameObject *>> objectPool;
  std::list<bms_parser::LongNote *>
      orphanLongNotes; // long note whose head is dead but tail is alive
  size_t currentTimelineIndex = 0;
};
class BMSRenderer {
private:
  ~BMSRenderer();
  TextView *judgeText = nullptr;
  TextView *scoreText = nullptr;
  std::map<int, LaneState> laneStates;
  JudgeResult latestJudgeResult = JudgeResult(None, 0);
  std::chrono::system_clock::time_point latestJudgeResultTime;
  int latestCombo = 0;
  int latestScore = 0;
  float noteImageHeight = 0;
  float noteImageWidth = 0;
  std::vector<bms_parser::TimeLine *> timelines;
  BMSRendererState state;
  float noteRenderWidth = 1.0f;
  float noteRenderHeight = 1.0f;
  float lowerBound = -1.0f;
  float upperBound = 20.0f;
  float judgeY = 0.0f;
  long long latePoorTiming;
  GameObject *getInstance(ObjectType type);
  void recycleInstance(ObjectType type, GameObject *object);
  void drawRect(RenderContext &context, float width, float height, float x,
                float y, Color color);
  void drawLaneBeam(RenderContext &context, int lane, const long long time);
  void drawJudgement(RenderContext context) const;
  void drawScore(RenderContext &context) const;
  void drawLongNote(RenderContext context, float headY, float tailY,
                    bms_parser::LongNote *const &head);
  void drawNormalNote(RenderContext &context, float y,
                      bms_parser::Note *const &note);
  bool isLeftScratch(int lane);
  bool isRightScratch(int lane);
  bool isScratch(int lane);
  float laneToX(int lane);
  bgfx::TextureHandle noteTexture = BGFX_INVALID_HANDLE;
  bgfx::TextureHandle noteTexture2 = BGFX_INVALID_HANDLE;
  bgfx::TextureHandle scratchTexture;
  bms_parser::Chart *chart;

public:
  void onLanePressed(int lane, const JudgeResult judge, long long time);
  void onLaneReleased(int lane, long long time);
  void onJudge(JudgeResult judgeResult, int combo, int score);
  explicit BMSRenderer(bms_parser::Chart *chart, long long latePoorTiming);

  void render(RenderContext &context, long long micro);
};
