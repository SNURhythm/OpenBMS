#include <algorithm>
#include "Judge.h"

Judge::Judge(const int Rank) {
  const int Clamped = clampRank(Rank);
  timingWindows = TimingWindowsByRank[Clamped];
}

bool Judge::checkRange(const long long Diff, const long long Early,
                       const long long Late) {
  return Early <= Diff && Diff <= Late;
}

JudgeResult Judge::judgeNow(const bms_parser::Note *Note,
                            const long long InputTime) {
  const auto &timeline = Note->Timeline;
  const long long diff = InputTime - timeline->Timing;
  // check range for each judgement
  for (const auto &window : timingWindows) {
    const auto &judgement = window.first;
    const auto &range = window.second;
    if (checkRange(diff, range.first, range.second)) {
      return JudgeResult{judgement, diff};
    }
  }

  return JudgeResult{None, diff};
}

int Judge::clampRank(const int rank) { return std::clamp(rank, 0, 3); };

std::string Judge::getRankDescription(const int Rank) {
  switch (clampRank(Rank)) {
  case 0:
    return "VERY HARD";
  case 1:
    return "HARD";
  case 2:
    return "NORMAL";
  case 3:
    return "EASY";
  default:
    return "EASY";
  }
}
