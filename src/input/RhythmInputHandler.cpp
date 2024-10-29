//
// Created by XF on 9/5/2024.
//

#include "RhythmInputHandler.h"
#include "SDLInputSource.h"
#include "SDLTouchInputSource.h"

#include <map>
void RhythmInputHandler::onKeyDown(int keyCode, KeySource keySource) {

  auto normalizedKeyCode = InputNormalizer::normalize(keyCode, keySource);
  SDL_Log("KeyDown: %d (%d)", keyCode, normalizedKeyCode);
  if (keyMap.contains(normalizedKeyCode)) {
    auto lane = keyMap[normalizedKeyCode];
    control->pressLane(lane);
  }
}
void RhythmInputHandler::onKeyUp(int keyCode, KeySource keySource) {
  SDL_Log("KeyUp: %d", keyCode);
  auto normalizedKeyCode = InputNormalizer::normalize(keyCode, keySource);
  if (keyMap.contains(normalizedKeyCode)) {
    auto lane = keyMap[normalizedKeyCode];
    control->releaseLane(lane);
  }
}
void RhythmInputHandler::onFingerDown(int fingerIndex, Vector3 location) {
  SDL_Log("FingerDown: %d, (%f, %f, %f)", fingerIndex, location.x, location.y,
          location.z);
}
void RhythmInputHandler::onFingerUp(int fingerIndex, Vector3 location) {
  SDL_Log("FingerUp: %d, (%f, %f, %f)", fingerIndex, location.x, location.y,
          location.z);
}
bool RhythmInputHandler::startListenSDL() {
  if (sdlInputSource != nullptr) {
    return false;
  }
  sdlInputSource = new SDLInputSource();
  sdlInputSource->setHandler(this);
  return sdlInputSource->startListen();
}
bool RhythmInputHandler::startListenTouch() {
  if (touchInputSource != nullptr) {
    return false;
  }
  touchInputSource = new SDLTouchInputSource();
  touchInputSource->setHandler(this);
  return touchInputSource->startListen();
}
void RhythmInputHandler::stopListen() {
  for (auto &input : {&sdlInputSource, &touchInputSource}) {
    if (*input != nullptr) {
      (*input)->stopListen();
      delete *input;
      *input = nullptr;
    }
  }
}
RhythmInputHandler::RhythmInputHandler(IRhythmControl *control,
                                       const bms_parser::ChartMeta &meta)
    : control(control) {
  std::map<int, std::map<SDL_Keycode, int>> DefaultKeyMap = {
      {7,
       {// keys: SDF, SPACE, JKL
        {SDL_KeyCode::SDLK_s, 0},
        {SDL_KeyCode::SDLK_d, 1},
        {SDL_KeyCode::SDLK_f, 2},
        {SDL_KeyCode::SDLK_SPACE, 3},
        {SDL_KeyCode::SDLK_j, 4},
        {SDL_KeyCode::SDLK_k, 5},
        {SDL_KeyCode::SDLK_l, 6},
        // scratch: LShift, RShift
        {SDL_KeyCode::SDLK_LSHIFT, 7},
        {SDL_KeyCode::SDLK_RSHIFT, 7}}},
      {5,
       {// keys: DF, SPACE, JK
        {SDL_KeyCode::SDLK_d, 0},
        {SDL_KeyCode::SDLK_f, 1},
        {SDL_KeyCode::SDLK_SPACE, 2},
        {SDL_KeyCode::SDLK_j, 3},
        {SDL_KeyCode::SDLK_k, 4},
        // scratch: LShift, RShift
        {SDL_KeyCode::SDLK_LSHIFT, 7},
        {SDL_KeyCode::SDLK_RSHIFT, 7}}},
      {14,
       {// keys: ZSXDCFV and MK,L.;/
        {SDL_KeyCode::SDLK_z, 0},
        {SDL_KeyCode::SDLK_s, 1},
        {SDL_KeyCode::SDLK_x, 2},
        {SDL_KeyCode::SDLK_d, 3},
        {SDL_KeyCode::SDLK_c, 4},
        {SDL_KeyCode::SDLK_f, 5},
        {SDL_KeyCode::SDLK_v, 6},
        {SDL_KeyCode::SDLK_m, 8},
        {SDL_KeyCode::SDLK_k, 9},
        {SDL_KeyCode::SDLK_COMMA, 10},
        {SDL_KeyCode::SDLK_l, 11},
        {SDL_KeyCode::SDLK_PERIOD, 12},
        {SDL_KeyCode::SDLK_SEMICOLON, 13},
        {SDL_KeyCode::SDLK_SLASH, 14},
        // Lscratch: LShift
        {SDL_KeyCode::SDLK_LSHIFT, 7},
        // Rscratch: RShift
        {SDL_KeyCode::SDLK_RSHIFT, 15}}},
      {10,
       {// keys: ZSXDC and ,l.;/
        {SDL_KeyCode::SDLK_z, 0},
        {SDL_KeyCode::SDLK_s, 1},
        {SDL_KeyCode::SDLK_x, 2},
        {SDL_KeyCode::SDLK_d, 3},
        {SDL_KeyCode::SDLK_c, 4},
        {SDL_KeyCode::SDLK_COMMA, 8},
        {SDL_KeyCode::SDLK_l, 9},
        {SDL_KeyCode::SDLK_PERIOD, 10},
        {SDL_KeyCode::SDLK_SEMICOLON, 11},
        {SDL_KeyCode::SDLK_SLASH, 12},
        // Lscratch: LShift
        {SDL_KeyCode::SDLK_LSHIFT, 7},
        // Rscratch: RShift
        {SDL_KeyCode::SDLK_RSHIFT, 15}}}};
  keyMap = DefaultKeyMap[meta.KeyMode];
}
