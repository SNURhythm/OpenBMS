//
// Created by XF on 9/2/2024.
//

#include "BMSRenderer.h"

#include "Judge.h"
#include "bgfx/bgfx.h"
#include "../../rendering/common.h"
#include "../../rendering/ShaderManager.h"
#include "stb_image.h"
#include "../../utils/SpriteLoader.h"

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
  SpriteLoader spriteLoader(PATH("assets/img/piano_w.png"));
  if (!spriteLoader.load()) {
    throw std::runtime_error("Failed to load piano_b.png");
  }

  // int width, height, channels;
  // unsigned char *data =
  //     stbi_load("assets/img/note.png", &width, &height, &channels, 4);
  // if (!data) {
  //   SDL_Log("Failed to load note texture");
  //   throw std::runtime_error("Failed to load note texture");
  // }
  int width = 128;
  int height = 40;
  auto data = spriteLoader.crop(0, 0, width, height);
  if (!data) {
    SDL_Log("Failed to load note texture");
    throw std::runtime_error("Failed to load note texture");
  }

  int channels = spriteLoader.getChannels();
  keyLaneCount = chart->Meta.GetKeyLaneCount();
  noteRenderWidth = 1.0f * 8.0f / chart->Meta.GetTotalLaneCount();
  noteImageHeight = height;
  noteImageWidth = width;
  noteRenderHeight = static_cast<float>(noteImageHeight) /
                     static_cast<float>(noteImageWidth) * noteRenderWidth;
  float offImageHeight = 12.0f;
  float onImageHeight = 24.0f;

  longBodyRenderHeightOff = static_cast<float>(offImageHeight) /
                            static_cast<float>(width) * noteRenderWidth;
  longBodyRenderHeightOn = static_cast<float>(onImageHeight) /
                           static_cast<float>(width) * noteRenderWidth;
  noteTexture =
      bgfx::createTexture2D(width, height, false, 1, bgfx::TextureFormat::RGBA8,
                            0, bgfx::copy(data, width * height * channels));
  SDL_free(data);
  data = spriteLoader.crop(0, 80, 128, 40);
  if (!data) {
    SDL_Log("Failed to load note texture");
    throw std::runtime_error("Failed to load note texture");
  }
  longHeadTexture =
      bgfx::createTexture2D(128, 40, false, 1, bgfx::TextureFormat::RGBA8, 0,
                            bgfx::copy(data, 128 * 40 * channels));
  SDL_free(data);
  data = spriteLoader.crop(0, 120, 128, 12);
  if (!data) {
    SDL_Log("Failed to load note texture");
    throw std::runtime_error("Failed to load note texture");
  }
  longBodyTextureOff =
      bgfx::createTexture2D(128, 12, false, 1, bgfx::TextureFormat::RGBA8, 0,
                            bgfx::copy(data, 128 * 12 * channels));
  SDL_free(data);
  data = spriteLoader.crop(0, 132, 128, 24);
  if (!data) {
    SDL_Log("Failed to load note texture");
    throw std::runtime_error("Failed to load note texture");
  }
  longBodyTextureOn =
      bgfx::createTexture2D(128, 24, false, 1, bgfx::TextureFormat::RGBA8, 0,
                            bgfx::copy(data, 128 * 24 * channels));
  SDL_free(data);
  data = spriteLoader.crop(0, 40, 128, 40);
  if (!data) {
    SDL_Log("Failed to load note texture");
    throw std::runtime_error("Failed to load note texture");
  }
  longTailTexture =
      bgfx::createTexture2D(128, 40, false, 1, bgfx::TextureFormat::RGBA8, 0,
                            bgfx::copy(data, 128 * 40 * channels));
  SDL_free(data);

  SpriteLoader spriteLoader2(PATH("assets/img/piano_b.png"));
  if (!spriteLoader2.load()) {
    throw std::runtime_error("Failed to load piano_w.png");
  }
  channels = spriteLoader2.getChannels();
  data = spriteLoader2.crop(0, 0, width, height);
  if (!data) {
    SDL_Log("Failed to load note texture");
    throw std::runtime_error("Failed to load note texture");
  }
  noteTexture2 =
      bgfx::createTexture2D(width, height, false, 1, bgfx::TextureFormat::RGBA8,
                            0, bgfx::copy(data, width * height * channels));
  SDL_free(data);
  data = spriteLoader2.crop(0, 80, 128, 40);
  if (!data) {
    SDL_Log("Failed to load note texture");
    throw std::runtime_error("Failed to load note texture");
  }
  longHeadTexture2 =
      bgfx::createTexture2D(128, 40, false, 1, bgfx::TextureFormat::RGBA8, 0,
                            bgfx::copy(data, 128 * 40 * channels));
  SDL_free(data);
  data = spriteLoader2.crop(0, 120, 128, 12);
  if (!data) {
    SDL_Log("Failed to load note texture");
    throw std::runtime_error("Failed to load note texture");
  }
  longBodyTextureOff2 =
      bgfx::createTexture2D(128, 12, false, 1, bgfx::TextureFormat::RGBA8, 0,
                            bgfx::copy(data, 128 * 12 * channels));
  SDL_free(data);
  data = spriteLoader2.crop(0, 132, 128, 24);
  longBodyTextureOn2 =
      bgfx::createTexture2D(128, 24, false, 1, bgfx::TextureFormat::RGBA8, 0,
                            bgfx::copy(data, 128 * 24 * channels));
  SDL_free(data);
  data = spriteLoader2.crop(0, 40, 128, 40);
  if (!data) {
    SDL_Log("Failed to load note texture");
    throw std::runtime_error("Failed to load note texture");
  }
  longTailTexture2 =
      bgfx::createTexture2D(128, 40, false, 1, bgfx::TextureFormat::RGBA8, 0,
                            bgfx::copy(data, 128 * 40 * channels));
  SDL_free(data);
  SpriteLoader spriteLoader3(PATH("assets/img/orange.png"));
  if (!spriteLoader3.load()) {
    throw std::runtime_error("Failed to load orange.png");
  }
  channels = spriteLoader3.getChannels();
  data = spriteLoader3.crop(0, 0, width, height);
  if (!data) {
    SDL_Log("Failed to load note texture");
    throw std::runtime_error("Failed to load note texture");
  }

  scratchTexture =
      bgfx::createTexture2D(width, height, false, 1, bgfx::TextureFormat::RGBA8,
                            0, bgfx::copy(data, width * height * channels));
  SDL_free(data);
  data = spriteLoader3.crop(0, 80, 128, 40);
  if (!data) {
    SDL_Log("Failed to load note texture");
    throw std::runtime_error("Failed to load note texture");
  }
  scratchLongHeadTexture =
      bgfx::createTexture2D(128, 40, false, 1, bgfx::TextureFormat::RGBA8, 0,
                            bgfx::copy(data, 128 * 40 * channels));
  SDL_free(data);
  data = spriteLoader3.crop(0, 120, 128, 12);
  if (!data) {
    SDL_Log("Failed to load note texture");
    throw std::runtime_error("Failed to load note texture");
  }
  scratchLongBodyTextureOff =
      bgfx::createTexture2D(128, 12, false, 1, bgfx::TextureFormat::RGBA8, 0,
                            bgfx::copy(data, 128 * 12 * channels));
  SDL_free(data);
  data = spriteLoader3.crop(0, 132, 128, 24);
  if (!data) {
    SDL_Log("Failed to load note texture");
    throw std::runtime_error("Failed to load note texture");
  }
  scratchLongBodyTextureOn =
      bgfx::createTexture2D(128, 24, false, 1, bgfx::TextureFormat::RGBA8, 0,
                            bgfx::copy(data, 128 * 24 * channels));
  SDL_free(data);
  data = spriteLoader3.crop(0, 40, 128, 40);
  if (!data) {
    SDL_Log("Failed to load note texture");
    throw std::runtime_error("Failed to load note texture");
  }
  scratchLongTailTexture =
      bgfx::createTexture2D(128, 40, false, 1, bgfx::TextureFormat::RGBA8, 0,
                            bgfx::copy(data, 128 * 40 * channels));
  SDL_free(data);
  judgeText = new TextView("assets/fonts/notosanscjkjp.ttf", 32);
  judgeText->setPosition(rendering::window_width / 2,
                         rendering::window_height / 2);
  judgeText->setAlign(TextView::CENTER);
  scoreText = new TextView("assets/fonts/notosanscjkjp.ttf", 32);
  scoreText->setPosition(0, rendering::window_height - 50);
  scoreText->setAlign(TextView::LEFT);
}
void BMSRenderer::drawJudgement(RenderContext context) const {
  if (state.latestJudgeResult.judgement == None) {
    return;
  }
  std::stringstream ss;
  ss << state.latestJudgeResult.toString();
  ss << " ";
  if (state.latestCombo > 0) {
    ss << state.latestCombo;
  }
  judgeText->setText(ss.str());

  judgeText->render(context);
}
void BMSRenderer::drawScore(RenderContext &context) const {
  std::stringstream ss;
  ss << "Score: " << state.latestScore;
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
  state.latestJudgeResult = judgeResult;
  state.latestJudgeResultTime = std::chrono::system_clock::now();
  state.latestCombo = combo;
  state.latestScore = score;
}
void BMSRenderer::drawLongNote(RenderContext context, const float headY,
                               float tailY, bms_parser::LongNote *const &head) {
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
  bodyObject->tileV = bodyHeight / (head->IsHolding ? longBodyRenderHeightOn
                                                    : longBodyRenderHeightOff);
  bodyObject->transform.position = {laneToX(head->Lane), startY, 0.0f};
  bodyObject->transform.rotation = Quaternion::fromEuler(0.0f, 0.0f, 0.0f);
  bodyObject->visible = true;

  SpriteObject *tailObject = state.noteObjectMap[head->Tail];
  tailObject->width = noteRenderWidth;
  tailObject->height = noteRenderHeight;
  tailObject->transform.position = {laneToX(head->Tail->Lane), tailY, 0.0f};
  tailObject->transform.rotation = Quaternion::fromEuler(0.0f, 0.0f, 0.0f);
  tailObject->visible = true;
  bgfx::TextureHandle bodyTexture{};
  bgfx::TextureHandle tailTexture{};
  bgfx::TextureHandle headTexture{};
  if (isScratch(head->Lane)) {
    headTexture = scratchLongHeadTexture;
    tailTexture = scratchLongTailTexture;
    if (head->IsHolding) {
      bodyTexture = scratchLongBodyTextureOn;
    } else {
      bodyTexture = scratchLongBodyTextureOff;
    }
  } else {
    headTexture = head->Lane % 2 == 0 ? longHeadTexture : longHeadTexture2;
    tailTexture = head->Lane % 2 == 0 ? longTailTexture : longTailTexture2;
    if (head->IsHolding) {
      bodyTexture =
          head->Lane % 2 == 0 ? longBodyTextureOn : longBodyTextureOn2;
    } else {
      bodyTexture =
          head->Lane % 2 == 0 ? longBodyTextureOff : longBodyTextureOff2;
    }
  }

  bodyObject->setTexture(bodyTexture);
  tailObject->setTexture(tailTexture);

  bodyObject->render(context);
  // TODO: render tail only in Charge Note mode.
  tailObject->render(context);

  if (head->IsPlayed)
    return;

  SpriteObject *headObject = state.noteObjectMap[head];
  headObject->width = noteRenderWidth;
  headObject->height = noteRenderHeight;
  headObject->transform.position = {laneToX(head->Lane), startY, 0.0f};
  headObject->transform.rotation = Quaternion::fromEuler(0.0f, 0.0f, 0.0f);
  headObject->visible = true;
  headObject->setTexture(headTexture);
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
  const auto &texture =
      isScratch(note->Lane)
          ? scratchTexture
          : (note->Lane % 2 == 0 ? noteTexture : noteTexture2);

  noteObject->setTexture(texture);
  noteObject->render(context);
}
void BMSRenderer::render(RenderContext &context, long long micro) {
  drawRect(context, 8.0f, noteRenderHeight, 0.0f, judgeY,
           Color(255, 255, 255, 255));
  float greenNumber = 400.0f;
  float hispeed = 240000.0f / chart->Meta.Bpm / greenNumber;
  float visibleLaneTop = 8.5f; // TODO: calculate from camera projection
  float visibleLaneBottom = judgeY;
  float rxhs = (visibleLaneTop - visibleLaneBottom) * hispeed;
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
          // when the previous timeline is stopped
          y += (timeLine->BeatPosition - prevTimeLine->BeatPosition) *
               prevTimeLine->Scroll * rxhs;
        } else {
          y += (timeLine->BeatPosition - prevTimeLine->BeatPosition) *
               prevTimeLine->Scroll * (timeLine->Timing - micro) /
               (timeLine->Timing - prevTimeLine->Timing -
                prevTimeLine->GetStopDuration()) *
               rxhs;
        }
      } else {
        y += timeLine->BeatPosition * (timeLine->Timing - micro) /
             timeLine->Timing * rxhs;
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
              if (longNote->Head == nullptr) {
                // ignore malformed chart: long note is not terminated
                continue;
              }
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
              if (longNote->Head == nullptr) {
                // ignore malformed chart: long note is not terminated
                continue;
              }
              // remove from orphan long note
              state.orphanLongNotes.remove(longNote->Head);
              // and from long note lookahead
              longNoteLookahead.erase(longNote->Head);
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
void BMSRenderer::reset() { state.reset(); }
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
  if (lane >= 8) {
    lane -=
        keyLaneCount == 14
            ? 1
            : (isRightScratch(lane) ? 5
                                    : 3); // skip left scratch index (7), since
                                          // 7 is already placed in the leftmost
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
void BMSRendererState::reset() {
  orphanLongNotes.clear();
  currentTimelineIndex = 0;
  latestJudgeResult = JudgeResult(None, 0);
  latestJudgeResultTime = std::chrono::system_clock::now();
  latestCombo = 0;
  latestScore = 0;
}
BMSRenderer::~BMSRenderer() {
  if (bgfx::isValid(noteTexture)) {
    bgfx::destroy(noteTexture);
  }
  if (bgfx::isValid(noteTexture2)) {
    bgfx::destroy(noteTexture2);
  }
  if (bgfx::isValid(longHeadTexture)) {
    bgfx::destroy(longHeadTexture);
  }
  if (bgfx::isValid(longBodyTextureOn)) {
    bgfx::destroy(longBodyTextureOn);
  }
  if (bgfx::isValid(longBodyTextureOff)) {
    bgfx::destroy(longBodyTextureOff);
  }
  if (bgfx::isValid(longTailTexture)) {
    bgfx::destroy(longTailTexture);
  }
  if (bgfx::isValid(longHeadTexture2)) {
    bgfx::destroy(longHeadTexture2);
  }
  if (bgfx::isValid(longBodyTextureOn2)) {
    bgfx::destroy(longBodyTextureOn2);
  }
  if (bgfx::isValid(longBodyTextureOff2)) {
    bgfx::destroy(longBodyTextureOff2);
  }
  if (bgfx::isValid(longTailTexture2)) {
    bgfx::destroy(longTailTexture2);
  }
  if (bgfx::isValid(scratchTexture)) {
    bgfx::destroy(scratchTexture);
  }
  if (bgfx::isValid(scratchLongHeadTexture)) {
    bgfx::destroy(scratchLongHeadTexture);
  }
  if (bgfx::isValid(scratchLongBodyTextureOn)) {
    bgfx::destroy(scratchLongBodyTextureOn);
  }
  if (bgfx::isValid(scratchLongBodyTextureOff)) {
    bgfx::destroy(scratchLongBodyTextureOff);
  }
  if (bgfx::isValid(scratchLongTailTexture)) {
    bgfx::destroy(scratchLongTailTexture);
  }
  delete judgeText;
  delete scoreText;
}
