#pragma once

#include "vlcpp/Instance.hpp"
class VLCInstance {
public:
  static VLCInstance &getInstance() {
    static VLCInstance instance;
    return instance;
  }

  VLC::Instance *getVLCInstance() { return vlcInstance.get(); }

private:
  VLCInstance() { vlcInstance = std::make_unique<VLC::Instance>(0, nullptr); }
  std::unique_ptr<VLC::Instance> vlcInstance;

  // Delete copy constructor and assignment operator
  VLCInstance(const VLCInstance &) = delete;
  VLCInstance &operator=(const VLCInstance &) = delete;
};
