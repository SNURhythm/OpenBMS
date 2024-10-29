//
// Created by XF on 9/4/2024.
//

#include "SDLInputSource.h"

int SDLInputSource::EventHandler(void *userdata, SDL_Event *event) {
  auto *InputSource = (SDLInputSource *)userdata;
  if (InputSource->handler == nullptr) {
    return 0;
  }
  switch (event->type) {
  case SDL_KEYDOWN:
    InputSource->handler->onKeyDown(event->key.keysym.scancode, ScanCode);
    break;
  case SDL_KEYUP:
    InputSource->handler->onKeyUp(event->key.keysym.scancode, ScanCode);
    break;
  }
  return 0;
}

SDLInputSource::SDLInputSource() { handler = nullptr; }

SDLInputSource::~SDLInputSource() {
  if (isListening) {
    SDL_DelEventWatch(EventHandler, this);
  }
}

bool SDLInputSource::startListen() {
  if (isListening) {
    return false;
  }
  isListening = true;

  SDL_AddEventWatch(EventHandler, this);
  return true;
}

void SDLInputSource::stopListen() {
  if (!isListening) {
    return;
  }
  isListening = false;
  SDL_DelEventWatch(EventHandler, this);
}

void SDLInputSource::setHandler(IInputHandler *handler) {
  this->handler = handler;
}