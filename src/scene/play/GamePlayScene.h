//
// Created by XF on 8/25/2024.
//

#pragma once
#include "RhythmState.h"
#include "../Scene.h"
#include "../../bms_parser.hpp"
#include "../../input/IRhythmControl.h"
#include "../../view/TextView.h"
#include "../../view/LinearLayout.h"

struct StartOptions {
  unsigned long long startPosition = 0;
  bool autoKeySound = false;
  bool autoPlay = false;
};
class RhythmInputHandler;
class BMSRenderer;
class GamePlayScene : public Scene, public IRhythmControl {
private:
  bms_parser::Chart *chart;
  bool isGamePaused = false;
  std::atomic_bool isCancelled = false;
  long long latePoorTiming;

public:
  GamePlayScene() = delete;

  explicit GamePlayScene(ApplicationContext &context, bms_parser::Chart *chart,
                         StartOptions options)
      : Scene(context), judge(chart->Meta.Rank), options(options) {
    this->chart = chart;
    latePoorTiming = judge.timingWindows[Bad].second;
  };
  void init() override;
  void update(float dt) override;
  void renderScene() override;
  void cleanupScene() override;
  int pressLane(int lane, double inputDelay) override;
  int pressLane(int mainLane, int compensateLane, double inputDelay) override;
  void releaseLane(int lane, double inputDelay) override;
  EventHandleResult handleEvents(SDL_Event &event) override;

private:
  void reset();
  LinearLayout *rootLayout = nullptr;
  Judge judge;
  StartOptions options;
  void checkPassedTimeline(long long time);
  void onJudge(const JudgeResult &judgeResult);
  JudgeResult pressNote(bms_parser::Note *note, long long pressedTime);
  void releaseNote(bms_parser::Note *Note, long long ReleasedTime);
  RhythmState *state = nullptr;
  BMSRenderer *renderer = nullptr;
  RhythmInputHandler *inputHandler = nullptr;
  std::map<int, bool> lanePressed;
  TextView *laneStateText = nullptr;
  std::mutex judgeMutex;
};
