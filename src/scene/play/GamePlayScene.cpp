//
// Created by XF on 8/25/2024.
//

#include "GamePlayScene.h"
#include "../../view/TextView.h"
#include "BMSRenderer.h"
#include "../../input/RhythmInputHandler.h"
void GamePlayScene::init() {
  auto chartNameText = new TextView("assets/fonts/notosanscjkjp.ttf", 32);
  chartNameText->setText("Selected: " + chart->Meta.Title);
  chartNameText->setPosition(10, 10);
  addView(chartNameText);
  renderer = new BMSRenderer(chart, judge.timingWindows[Bad].second);
  state = new RhythmState(chart, false);
  context.jukebox.schedule(*chart, false, isCancelled);
  context.jukebox.play();
  state->isPlaying = true;
  inputHandler = new RhythmInputHandler(this, chart->Meta);
  inputHandler->startListenSDL();
  laneStateText = new TextView("assets/fonts/notosanscjkjp.ttf", 32);
  laneStateText->setPosition(100, 100);
  for (const auto &lane : chart->Meta.GetTotalLaneIndices()) {
    lanePressed[lane] = false;
  }
  context.jukebox.onTick([this](long long time) {
    if (state != nullptr && state->isPlaying) {
      checkPassedTimeline(time);
      if (state->passedMeasureCount == chart->Measures.size()) {
        SDL_Log("All measures passed");
      }
    }
  });
}
void GamePlayScene::update(float dt) {}

void GamePlayScene::renderScene() {
  RenderContext renderContext;
  renderer->render(renderContext, context.jukebox.getTimeMicros());
  context.jukebox.render();
  std::string str;
  for (auto &[lane, pressed] : lanePressed) {
    str += std::to_string(pressed) + "\n";
  }
  laneStateText->setText(str);
  laneStateText->render(renderContext);
}
void GamePlayScene::cleanupScene() { context.jukebox.removeOnTick(); }
int GamePlayScene::pressLane(int lane, double inputDelay) {
  return pressLane(lane, lane, inputDelay);
}
int GamePlayScene::pressLane(int mainLane, int compensateLane,
                             double inputDelay) {
  std::vector<int> candidates;

  for (auto Lane : mainLane == compensateLane
                       ? std::initializer_list<int>{mainLane}
                       : std::initializer_list<int>{mainLane, compensateLane}) {
    if (!lanePressed.contains(Lane) || lanePressed[Lane]) {
      continue;
    }
    candidates.push_back(Lane);
  }
  if (candidates.empty()) {
    return mainLane;
  }

  if (isGamePaused) {
    return mainLane;
  }

  if (state == nullptr) {
    lanePressed[mainLane] = true;
    renderer->onLanePressed(
        mainLane, JudgeResult(None, 0),
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count());
    return mainLane;
  }
  if (!state->isPlaying) {
    lanePressed[mainLane] = true;
    return mainLane;
  }

  const auto &measures = chart->Measures;
  const auto pressedTime = context.jukebox.getTimeMicros() -
                           static_cast<long long>(inputDelay * 1000000);
  for (size_t i = state->passedMeasureCount; i < measures.size(); i++) {
    const bool isFirstMeasure = i == state->passedMeasureCount;
    const auto &measure = measures[i];

    for (size_t j = isFirstMeasure ? state->passedTimelineCount : 0;
         j < measure->TimeLines.size(); j++) {
      const auto &timeline = measure->TimeLines[j];
      if (timeline->Timing < pressedTime - latePoorTiming) {
        continue;
      }
      for (auto lane : candidates) {
        const auto &note = timeline->Notes[lane];
        if (note == nullptr) {
          continue;
        }
        if (note->IsPlayed) {
          continue;
        }
        if (note->IsLandmineNote()) {
          continue;
        }
        if (judge.judgeNow(note, pressedTime).judgement == None) {
          continue;
        }
        const JudgeResult judgement = pressNote(note, pressedTime);
        lanePressed[lane] = true;
        renderer->onLanePressed(
            lane, judgement,
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count());
        return lane;
      }
    }
  }
  lanePressed[mainLane] = true;
  renderer->onLanePressed(
      mainLane, JudgeResult(None, 0),
      std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count());
  return mainLane;
}
void GamePlayScene::releaseLane(int lane, double inputDelay) {
  if (!lanePressed.contains(lane) || !lanePressed[lane]) {
    return;
  }
  lanePressed[lane] = false;
  renderer->onLaneReleased(
      lane, std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count());
  const auto releasedTime = context.jukebox.getTimeMicros() -
                            static_cast<long long>(inputDelay * 1000000);

  if (state == nullptr) {
    return;
  }
  if (!state->isPlaying) {
    return;
  }

  const auto &Measures = chart->Measures;

  for (size_t i = state->passedMeasureCount; i < Measures.size(); i++) {
    const bool isFirstMeasure = i == state->passedMeasureCount;
    const auto &measure = Measures[i];
    for (size_t j = isFirstMeasure ? state->passedTimelineCount : 0;
         j < measure->TimeLines.size(); j++) {
      const auto &Timeline = measure->TimeLines[j];
      if (Timeline->Timing < releasedTime - latePoorTiming) {
        continue;
      }
      const auto &note = Timeline->Notes[lane];
      if (note == nullptr) {
        continue;
      }
      if (note->IsPlayed) {
        continue;
      }
      releaseNote(note, releasedTime);
      return;
    }
  }
}
void GamePlayScene::checkPassedTimeline(long long time) {
  auto measures = chart->Measures;
  if (state == nullptr) {
    return;
  }
  int totalLoopCount = 0;
  for (size_t i = state->passedMeasureCount; i < measures.size(); i++) {
    const bool isFirstMeasure = i == state->passedMeasureCount;
    const auto &measure = measures[i];
    for (size_t j = isFirstMeasure ? state->passedTimelineCount : 0;
         j < measure->TimeLines.size(); j++) {
      totalLoopCount++;
      const auto &timeline = measure->TimeLines[j];
      if (timeline->Timing < time - latePoorTiming) {
        if (isFirstMeasure) {
          state->passedTimelineCount++;
        }
        // make remaining notes POOR
        for (const auto &note : timeline->Notes) {
          if (note == nullptr) {
            continue;
          }
          if (note->IsPlayed) {
            continue;
          }
          if (note->IsLandmineNote()) {
            continue;
          }
          if (note->IsLongNote()) {
            const auto &longNote = static_cast<bms_parser::LongNote *>(note);
            if (!longNote->IsTail()) {
              longNote->MissPress(time);
            }
          }
          const auto poorResult = JudgeResult(Poor, time - timeline->Timing);
          onJudge(poorResult);
        }
      } else if (timeline->Timing <= time) {
        // auto-release long notes
        for (const auto &note : timeline->Notes) {
          if (note == nullptr) {
            continue;
          }
          if (note->IsPlayed) {
            continue;
          }
          if (note->IsLandmineNote()) {
            // TODO: if lane is being pressed, detonate landmine
            continue;
          }
          if (note->IsLongNote()) {
            const auto &longNote = static_cast<bms_parser::LongNote *>(note);
            if (longNote->IsTail()) {
              if (!longNote->IsHolding) {
                continue;
              }
              longNote->Release(time);
              const auto judgeResult =
                  judge.judgeNow(longNote->Head, longNote->Head->PlayedTime);
              onJudge(judgeResult);
              if (options.autoPlay) {
                renderer->onLaneReleased(
                    note->Lane,
                    std::chrono::duration_cast<std::chrono::microseconds>(
                        std::chrono::system_clock::now().time_since_epoch())
                        .count());
              }
              continue;
            }
          }
          if (options.autoPlay) // NormalNote or LongNote's head
          {
            const JudgeResult judgeResult = pressNote(note, time);
            renderer->onLanePressed(note->Lane, judgeResult, time);
            if (!note->IsLongNote()) {
              renderer->onLaneReleased(
                  note->Lane,
                  std::chrono::duration_cast<std::chrono::microseconds>(
                      std::chrono::system_clock::now().time_since_epoch())
                      .count());
            }
          }
        }
      } else {
        i = measures.size();
        break;
      }
    }
    if (state->passedTimelineCount == measure->TimeLines.size() &&
        isFirstMeasure) {
      state->passedMeasureCount++;
      state->passedTimelineCount = 0;
    }
  }
}

void GamePlayScene::onJudge(const JudgeResult &judgeResult) {
  std::lock_guard<std::mutex> lock(judgeMutex);
  state->latestJudgeResult = judgeResult;

  state->judgeCount[judgeResult.judgement]++;
  if (judgeResult.isComboBreak()) {
    state->combo = 0;
    state->comboBreak++;
  } else if (judgeResult.judgement != Kpoor) {
    state->combo++;
  }
  renderer->onJudge(judgeResult, state->combo);
  SDL_Log("Judge: %s, Combo: %d, Diff: %lld", judgeResult.toString().c_str(),
          state->combo, judgeResult.Diff);
  // CurrentRhythmHUD->OnJudge(state);
  // UE_LOG(LogTemp, Warning, TEXT("Judge: %s, Combo: %d, Diff: %lld"),
  // *JudgeResult.ToString(), state->Combo, JudgeResult.Diff);
}

JudgeResult GamePlayScene::pressNote(bms_parser::Note *note,
                                     long long pressedTime) {
  if (note->Wav != bms_parser::Parser::NoWav && !options.autoKeySound) {
    context.jukebox.playKeySound(note->Wav);
  }
  const auto judgeResult = judge.judgeNow(note, pressedTime);
  if (judgeResult.judgement != None) {
    if (judgeResult.isNotePlayed()) {
      // TODO: play keybomb
      if (note->IsLongNote()) {
        if (const auto &longNote = static_cast<bms_parser::LongNote *>(note);
            !longNote->IsTail()) {
          longNote->Press(pressedTime);
        }
        return judgeResult;
      }
      note->Press(pressedTime);
    }
    onJudge(judgeResult);
  }
  return judgeResult;
}

void GamePlayScene::releaseNote(bms_parser::Note *Note,
                                long long ReleasedTime) {
  if (!Note->IsLongNote()) {
    return;
  }
  const auto &LongNote = static_cast<bms_parser::LongNote *>(Note);
  if (!LongNote->IsTail()) {
    return;
  }
  if (!LongNote->IsHolding) {
    return;
  }
  LongNote->Release(ReleasedTime);
  const auto judgeResult = judge.judgeNow(LongNote, ReleasedTime);
  // if tail judgement is not good/great/pgreat, make it bad
  if (judgeResult.judgement == None || judgeResult.judgement == Kpoor ||
      judgeResult.judgement == Poor) {
    onJudge(JudgeResult(Bad, judgeResult.Diff));
    return;
  }
  // otherwise, follow the head's judgement
  const auto HeadJudgeResult =
      judge.judgeNow(LongNote->Head, LongNote->Head->PlayedTime);
  onJudge(HeadJudgeResult);
}