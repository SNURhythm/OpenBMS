//
// Created by XF on 9/2/2024.
//

#include "BMSRenderer.h"

#include "Judge.h"
#include "bgfx/bgfx.h"
#include "../../rendering/common.h"
#include "../../rendering/ShaderManager.h"
#include "stb_image.h"

#include <assert.h>
#include <sstream>
BMSRenderer::BMSRenderer(bms_parser::Chart *chart, long long latePoorTiming)
    : latePoorTiming(latePoorTiming), chart(chart) {
  for (auto lane : chart->Meta.GetTotalLaneIndices()) {
    laneStates[lane] = LaneState();
  }
  // flatten timeline
  for (const auto &measure : chart->Measures) {
    for (const auto &timeLine : measure->TimeLines) {
      timelines.push_back(timeLine);
    }
  }
  int width, height, channels;
  unsigned char *data =
      stbi_load("assets/img/note.png", &width, &height, &channels, 4);
  if (!data) {
    SDL_Log("Failed to load note texture");
    throw std::runtime_error("Failed to load note texture");
  }
  noteImageHeight = height;
  noteImageWidth = width;
  noteRenderHeight = static_cast<float>(noteImageHeight) /
                     static_cast<float>(noteImageWidth) * noteRenderWidth;
  noteTexture =
      bgfx::createTexture2D(width, height, false, 1, bgfx::TextureFormat::RGBA8,
                            0, bgfx::copy(data, width * height * 4));
  stbi_image_free(data);
  data = stbi_load("assets/img/note2.png", &width, &height, &channels, 4);
  if (!data) {
    SDL_Log("Failed to load note texture");
    throw std::runtime_error("Failed to load note texture");
  }
  noteTexture2 =
      bgfx::createTexture2D(width, height, false, 1, bgfx::TextureFormat::RGBA8,
                            0, bgfx::copy(data, width * height * 4));
  stbi_image_free(data);
  data = stbi_load("assets/img/scratch.png", &width, &height, &channels, 4);
  if (!data) {
    SDL_Log("Failed to load note texture");
    throw std::runtime_error("Failed to load note texture");
  }
  scratchTexture =
      bgfx::createTexture2D(width, height, false, 1, bgfx::TextureFormat::RGBA8,
                            0, bgfx::copy(data, width * height * 4));
  stbi_image_free(data);

  judgeText = new TextView("assets/fonts/notosanscjkjp.ttf", 32);
  judgeText->setPosition(rendering::window_width / 2,
                         rendering::window_height / 2);
  judgeText->setAlign(TextView::CENTER);
  scoreText = new TextView("assets/fonts/notosanscjkjp.ttf", 32);
  scoreText->setPosition(0, rendering::window_height - 50);
  scoreText->setAlign(TextView::LEFT);
}
void BMSRenderer::drawJudgement(RenderContext context) const {
  if (latestJudgeResult.judgement == None) {
    return;
  }
  std::stringstream ss;
  ss << latestJudgeResult.toString();
  ss << " ";
  if (latestCombo > 0) {
    ss << latestCombo;
  }
  judgeText->setText(ss.str());

  judgeText->render(context);
}
void BMSRenderer::drawScore(RenderContext &context) const {
  std::stringstream ss;
  ss << "Score: " << latestScore;
  scoreText->setText(ss.str());
  scoreText->render(context);
}

void BMSRenderer::onLanePressed(int lane, const JudgeResult judge,
                                long long time) {
  laneStates[lane].isPressed = true;
  laneStates[lane].lastPressedJudge = judge;
  laneStates[lane].lastStateTime = time;
}

void BMSRenderer::onLaneReleased(int lane, long long time) {
  laneStates[lane].isPressed = false;
  laneStates[lane].lastStateTime = time;
}
void BMSRenderer::onJudge(JudgeResult judgeResult, int combo, int score) {
  if (judgeResult.judgement == None) {
    return;
  }
  latestJudgeResult = judgeResult;
  latestJudgeResultTime = std::chrono::system_clock::now();
  latestCombo = combo;
  latestScore = score;
}
void BMSRenderer::drawLongNote(RenderContext context, const float headY, float tailY,
                               bms_parser::LongNote *const &head) {
  // assert head
  assert(!head->IsTail() && "head is tail");
  if (head->Tail->IsPlayed)
    return;
  float startY = head->IsPlayed ? judgeY : headY;
  const float bodyHeight = tailY - startY;
  const float bodyWidth = noteRenderWidth;
  if (!state.noteObjectMap.contains(head)) {
    state.noteObjectMap[head] =
        dynamic_cast<SpriteObject *>(getInstance(ObjectType::Note));
  }
  if (!state.noteObjectMap.contains(head->Tail)) {
    state.noteObjectMap[head->Tail] =
        dynamic_cast<SpriteObject *>(getInstance(ObjectType::Note));
  }
  if (!state.longBodyObjectMap.contains(head)) {
    state.longBodyObjectMap[head] =
        dynamic_cast<SpriteObject *>(getInstance(ObjectType::LongBody));
  }

  SpriteObject *bodyObject = state.longBodyObjectMap[head];
  bodyObject->width = bodyWidth;
  bodyObject->height = bodyHeight;
  bodyObject->tileV = bodyHeight / noteRenderHeight;
  bodyObject->transform.position = {laneToX(head->Lane), startY, 0.0f};
  bodyObject->transform.rotation = Quaternion::fromEuler(0.0f, 0.0f, 0.0f);
  bodyObject->visible = true;

  SpriteObject *tailObject = state.noteObjectMap[head->Tail];
  tailObject->width = noteRenderWidth;
  tailObject->height = noteRenderHeight;
  tailObject->transform.position = {laneToX(head->Tail->Lane), tailY, 0.0f};
  tailObject->transform.rotation = Quaternion::fromEuler(0.0f, 0.0f, 0.0f);
  tailObject->visible = true;
  auto &texture = isScratch(head->Lane)
                      ? scratchTexture
                      : (head->Lane % 2 == 0 ? noteTexture : noteTexture2);

  bodyObject->setTexture(texture);
  tailObject->setTexture(texture);

  bodyObject->render(context);
  tailObject->render(context);

  if (head->IsPlayed)
    return;

  SpriteObject *headObject = state.noteObjectMap[head];
  headObject->width = noteRenderWidth;
  headObject->height = noteRenderHeight;
  headObject->transform.position = {laneToX(head->Lane), startY, 0.0f};
  headObject->transform.rotation = Quaternion::fromEuler(0.0f, 0.0f, 0.0f);
  headObject->visible = true;
  headObject->setTexture(texture);
  headObject->render(context);
}
void BMSRenderer::drawNormalNote(RenderContext &context, float y,
                                 bms_parser::Note *const &note) {
  if (note->IsPlayed)
    return;
  if (!state.noteObjectMap.contains(note)) {
    state.noteObjectMap[note] =
        dynamic_cast<SpriteObject *>(getInstance(ObjectType::Note));
  }
  SpriteObject *noteObject = state.noteObjectMap[note];
  noteObject->width = noteRenderWidth;
  noteObject->height = noteRenderHeight;

  auto x = laneToX(note->Lane);
  noteObject->transform.position = {x, y, 0.0f};
  noteObject->transform.rotation = Quaternion::fromEuler(0.0f, 0.0f, 0.0f);
  noteObject->visible = true;
  const auto &texture = isScratch(note->Lane)
                      ? scratchTexture
                      : (note->Lane % 2 == 0 ? noteTexture : noteTexture2);

  noteObject->setTexture(texture);
  noteObject->render(context);
}
void BMSRenderer::render(RenderContext &context, long long micro) {
  drawRect(context, 8.0f, noteRenderHeight, 0.0f, judgeY,
           Color(255, 255, 255, 255));
  float y = judgeY;
  std::map<bms_parser::LongNote *, float> longNoteLookahead;
  for (auto &orphanLongNote : state.orphanLongNotes) {
    longNoteLookahead[orphanLongNote] = lowerBound;
  }
  // render timeline
  for (size_t i = state.currentTimelineIndex;
       i < timelines.size() && y < upperBound; i++) {
    const auto &timeLine = timelines[i];
    if (timeLine->Timing >= micro) {
      if (y < judgeY)
        y = judgeY;
      if (i > 0) {
        if (const auto &prevTimeLine = timelines[i - 1];
            prevTimeLine->Timing + prevTimeLine->GetStopDuration() > micro) {
          y += (timeLine->BeatPosition - prevTimeLine->BeatPosition) *
               prevTimeLine->Scroll * 10.0f;
        } else {
          y += (timeLine->BeatPosition - prevTimeLine->BeatPosition) *
               prevTimeLine->Scroll * (timeLine->Timing - micro) /
               (timeLine->Timing - prevTimeLine->Timing -
                prevTimeLine->GetStopDuration()) *
               10.0f;
        }
      } else {
        y += timeLine->BeatPosition * (timeLine->Timing - micro) /
             timeLine->Timing * 10.0f;
      }

      if (timeLine->IsFirstInMeasure) {
        // render measure line
        drawRect(context, 8.0f, 0.05f, 0.0f, y, Color(255, 255, 255, 128));
      }
    } else if (timeLine->Timing >= micro - latePoorTiming) {
      y = judgeY + (micro - timeLine->Timing) /
                       static_cast<float>(latePoorTiming) * lowerBound;
    } else {
      state.currentTimelineIndex = i;
    }
    //    SDL_Log("BeatPosition: %f", timeLine->BeatPosition);
    // render notes
    for (const auto &note : timeLine->Notes) {
      if (note != nullptr) {
        if (timeLine->Timing >= micro - latePoorTiming) {
          if (note->IsDead) {
            continue;
          }
          // render note
          if (note->IsLongNote()) {
            if (auto *longNote = dynamic_cast<bms_parser::LongNote *>(note);
                longNote->IsTail()) {
              // find head's y
              if (auto it = longNoteLookahead.find(longNote->Head);
                  it != longNoteLookahead.end()) {
                drawLongNote(context, it->second, y, longNote->Head);
                // remove from lookahead
                longNoteLookahead.erase(longNote->Head);
              } else {
                drawLongNote(context, lowerBound, y, longNote->Head);
              }
            } else {
              longNoteLookahead[longNote] = y;
            }
          } else {
            drawNormalNote(context, y, note);
          }
        } else {
          if (note->IsLongNote()) {
            if (auto *longNote = dynamic_cast<bms_parser::LongNote *>(note);
                longNote->IsTail()) {
              // remove from orphan long note
              state.orphanLongNotes.remove(longNote->Head);
            } else {
              // add to orphan long note
              state.orphanLongNotes.push_back(longNote);
            }
          }
          if (state.noteObjectMap.find(note) != state.noteObjectMap.end()) {
            recycleInstance(ObjectType::Note, state.noteObjectMap[note]);
            state.noteObjectMap.erase(note);
          }
        }
      }
    }
    // render landmine notes
    for (const auto &note : timeLine->LandmineNotes) {
      if (note != nullptr) {
        // render note
      }
    }
  }

  // render leftover long notes
  for (const auto &pair : longNoteLookahead) {
    if (pair.first->Timeline->Timing < micro - latePoorTiming) continue;
    drawLongNote(context, pair.second, upperBound, pair.first);
  }
  // render lane beams
  for (auto &laneState : laneStates) {
    drawLaneBeam(context, laneState.first,
                 std::chrono::duration_cast<std::chrono::microseconds>(
                     std::chrono::system_clock::now().time_since_epoch())
                     .count());
  }

  // render judgement
  drawJudgement(context);
  drawScore(context);
}
void BMSRenderer::drawRect(RenderContext &context, float width, float height,
                           float x, float y, Color color) {
  // draw judge line
  bgfx::TransientVertexBuffer tvb{};
  bgfx::TransientIndexBuffer tib{};

  // Define the vertex layout
  bgfx::VertexLayout layout = rendering::PosColorVertex::ms_decl;

  bgfx::allocTransientVertexBuffer(&tvb, 4, layout);
  bgfx::allocTransientIndexBuffer(&tib, 6);

  auto *vertices = (rendering::PosColorVertex *)tvb.data;
  auto *index = (uint16_t *)tib.data;

  // Define the corners of the rectangle in local space (2D in X-Y plane)
  uint32_t abgr = color.toABGR();
  vertices[0] = {x, y, 0.0f, abgr};
  vertices[1] = {x + width, y, 0.0f, abgr};
  vertices[2] = {x + width, y + height, 0.0f, abgr};
  vertices[3] = {x, y + height, 0.0f, abgr};

  // Set up indices for two triangles (quad)
  index[0] = 0;
  index[1] = 1;
  index[2] = 2;
  index[3] = 2;
  index[4] = 3;
  index[5] = 0;

  // Set up state (e.g., render state, texture, shaders)
  uint64_t state = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
                   BGFX_STATE_BLEND_ALPHA | BGFX_STATE_MSAA;
  bgfx::setState(state);

  // Set the vertex and index buffers
  bgfx::setVertexBuffer(0, &tvb);
  bgfx::setIndexBuffer(&tib);

  // Submit the draw call
  bgfx::submit(
      rendering::main_view,
      rendering::ShaderManager::getInstance().getProgram(SHADER_SIMPLE));
}
void BMSRenderer::drawLaneBeam(RenderContext &context, int lane,
                               const long long time) {
  if (laneStates[lane].lastStateTime == -1) {
    return;
  }
  const auto &laneState = laneStates[lane];
  // alpha
  double alpha;
  if (laneState.isPressed) {
    alpha = 0.5;
  } else {
    // fade out
    alpha = 0.5 - (time - laneState.lastStateTime) / 1000000.0 / 1.0;
  }
  if (alpha <= 0.0) {
    return;
  }
  if (alpha > 1.0) {
    alpha = 1.0;
  }
  auto color = Color(255, 255, 255, 255 * alpha);

  if (laneState.lastPressedJudge.judgement == PGreat) {
    color = Color(255, 128, 0, 255 * alpha);
  } else if (laneState.lastPressedJudge.judgement == None) {
    color = Color(255, 255, 255, 255 * alpha);
  } else {
    color = laneState.lastPressedJudge.Diff > 0 ? Color(255, 0, 0, 255 * alpha)
                                                : Color(0, 0, 255, 255 * alpha);
  }
  drawRect(context, noteRenderWidth, 10.0f, laneToX(lane), 0.0f, color);
}

inline bool BMSRenderer::isLeftScratch(int lane) { return lane == 7; }
inline bool BMSRenderer::isRightScratch(int lane) { return lane == 15; }
inline bool BMSRenderer::isScratch(int lane) {
  return isLeftScratch(lane) || isRightScratch(lane);
}
inline float BMSRenderer::laneToX(int lane) {
  if (isLeftScratch(lane)) {
    return 0.0f;
  }

  return (lane + 1) * noteRenderWidth;
}
GameObject *BMSRenderer::getInstance(ObjectType type) {
  if (state.objectPool.find(type) == state.objectPool.end()) {
    state.objectPool[type] = std::queue<GameObject *>();
  }
  if (!state.objectPool[type].empty()) {
    GameObject *object = state.objectPool[type].front();
    state.objectPool[type].pop();
    return object;
  }
  switch (type) {
  case Note:
  case LongBody:
    return new SpriteObject(noteTexture);
  default:
    throw std::runtime_error("Unknown object type");
  }
}
void BMSRenderer::recycleInstance(ObjectType type, GameObject *object) {
  object->visible = false;
  if (!state.objectPool.contains(type)) {
    state.objectPool[type] = std::queue<GameObject *>();
  }
  state.objectPool[type].push(object);
}
BMSRendererState::~BMSRendererState() {
  for (auto &pair : objectPool) {
    while (!pair.second.empty()) {
      delete pair.second.front();
      pair.second.pop();
    }
  }
}
BMSRenderer::~BMSRenderer() {
  if (bgfx::isValid(noteTexture)) {
    bgfx::destroy(noteTexture);
  }
  if (bgfx::isValid(noteTexture2)) {
    bgfx::destroy(noteTexture2);
  }
  delete judgeText;
  delete scoreText;
}
