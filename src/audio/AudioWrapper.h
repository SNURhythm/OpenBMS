#pragma once

#include "miniaudio.h"
#include <map>
#include <string>
#include <vector>
#include "../path.h"

class AudioWrapper {
public:
  AudioWrapper();
  ~AudioWrapper();

  bool loadSound(const path_t &path);   // Load and store a sound
  void stopSounds();                  // Stop all sounds
  void unloadSound(const path_t &path); // Unload a sound
  void unloadSounds();                // Unload all sounds
  void preloadSounds(
      const std::vector<path_t> &paths); // Preload multiple sounds
  bool playSound(const path_t &path);     // Play a preloaded sound

private:
  ma_engine engine;
  ma_engine_config engineConfig;
  // sound group
  ma_sound_group soundGroup;
  ma_sound_group_config soundGroupConfig;
  std::map<path_t, ma_device> devices;
};
