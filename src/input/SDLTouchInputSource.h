#pragma once
#include "IInputSource.h"

class SDLTouchInputSource : public IInputSource {
public:
  static int EventHandler(void *userdata, SDL_Event *event);
  IInputHandler *handler = nullptr;
  SDLTouchInputSource();
  ~SDLTouchInputSource() override;
  bool startListen() override;
  void stopListen() override;
  void setHandler(IInputHandler *handler) override;
  bool isListening = false;
};
