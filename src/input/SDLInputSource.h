//
// Created by XF on 9/4/2024.
//

#pragma once
#include "IInputSource.h"
class SDLInputSource : public IInputSource {
public:
  IInputHandler *hander = nullptr;
  SDLInputSource();
  ~SDLInputSource() override;
  bool startListen() override;
  void stopListen() override;
  void setHandler(IInputHandler *handler) override;
  bool isListening = false;

private:
};
