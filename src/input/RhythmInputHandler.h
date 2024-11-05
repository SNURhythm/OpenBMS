//
// Created by XF on 9/5/2024.
//

#pragma once

#include "IInputHandler.h"
#include "../bms_parser.hpp"
#include "IRhythmControl.h"
#include "IInputSource.h"
class RhythmInputHandler : public IInputHandler {
private:
  IInputSource *sdlInputSource = nullptr;
  IInputSource *touchInputSource = nullptr;
  int keyMode;
  std::map<int, int> fingerToLane;

public:
  IRhythmControl *control;
  RhythmInputHandler(IRhythmControl *control,
                     const bms_parser::ChartMeta &meta);
  void onKeyDown(int keyCode, KeySource keySource) override;
  void onKeyUp(int KeyCode, KeySource Source) override;
  void onFingerDown(int fingerIndex, Vector3 location) override;
  void onFingerUp(int fingerIndex, Vector3 location) override;
  bool startListenSDL();
  bool startListenTouch();
  void stopListen();
  int touchToLane(Vector3 location);
  std::map<SDL_Keycode, int> keyMap;
};
