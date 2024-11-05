#include "SDLTouchInputSource.h"
#include "../rendering/common.h"
int SDLTouchInputSource::EventHandler(void *userdata, SDL_Event *event) {
  auto *InputSource = (SDLTouchInputSource *)userdata;
  if (InputSource->handler == nullptr) {
    return 0;
  }
  switch (event->type) {
  case SDL_FINGERDOWN: {
    float screenX = event->tfinger.x * rendering::window_width;
    float screenY = event->tfinger.y * rendering::window_height;
    InputSource->handler->onFingerDown(event->tfinger.fingerId,
                                       Vector3(screenX, screenY, 0.0f));
    break;
  }
  case SDL_FINGERUP: {
    float screenX = event->tfinger.x * rendering::window_width;
    float screenY = event->tfinger.y * rendering::window_height;
    InputSource->handler->onFingerUp(event->tfinger.fingerId,
                                     Vector3(screenX, screenY, 0.0f));
    break;
  }
    // emulate touch with click
  case SDL_MOUSEBUTTONDOWN:
    InputSource->handler->onFingerDown(
        0, Vector3(event->button.x, event->button.y, 0.0f));
    break;
  case SDL_MOUSEBUTTONUP:
    InputSource->handler->onFingerUp(
        0, Vector3(event->button.x, event->button.y, 0.0f));
    break;
    // case SDL_FINGERMOTION:
    //   InputSource->handler->onFingerMove(
    //       event->tfinger.fingerId,
    //       Vector3(event->tfinger.x, event->tfinger.y, 0.0f));
    //   break;
  }
  return 0;
}

SDLTouchInputSource::SDLTouchInputSource() { handler = nullptr; }

SDLTouchInputSource::~SDLTouchInputSource() {
  if (isListening) {
    SDL_DelEventWatch(EventHandler, this);
  }
}

bool SDLTouchInputSource::startListen() {
  if (isListening) {
    return false;
  }
  isListening = true;

  SDL_AddEventWatch(EventHandler, this);
  return true;
}

void SDLTouchInputSource::stopListen() {
  if (!isListening) {
    return;
  }
  isListening = false;
  SDL_DelEventWatch(EventHandler, this);
}

void SDLTouchInputSource::setHandler(IInputHandler *handler) {
  this->handler = handler;
}