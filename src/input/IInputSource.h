//
// Created by XF on 9/4/2024.
//

#pragma once
#include "IInputHandler.h"
class IInputSource {
public:
  virtual ~IInputSource() = default;
  virtual bool startListen() = 0;
  virtual void stopListen() = 0;
  virtual void setHandler(IInputHandler *handler) = 0;
};
