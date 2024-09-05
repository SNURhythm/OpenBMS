#pragma once
#include "../../bms_parser.hpp"
#include "Judge.h"
class RhythmState {
public:
  Judge *judge;
  bool isPlaying = false;

  size_t passedMeasureCount = 0;
  size_t passedTimelineCount = 0;

  int combo = 0;
  int comboBreak = 0;
  JudgeResult latestJudgeResult = JudgeResult(None, 0);
  // judge count. default 0
  std::map<Judgement, int> judgeCount;

  explicit RhythmState(const bms_parser::Chart *Chart, bool addReadyMeasure) {
    judge = new Judge(Chart->Meta.Rank);
    for (int i = 0; i < JudgementCount; i++) {
      judgeCount[static_cast<Judgement>(i)] = 0;
    }
  }

  ~RhythmState() { delete judge; }

private:
  long long firstTiming = 0;
};