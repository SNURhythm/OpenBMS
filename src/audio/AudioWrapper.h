#pragma once

#include "miniaudio.h"
#include <map>
#include <string>
#include <vector>

class AudioWrapper {
public:
  AudioWrapper();
  ~AudioWrapper();

  bool loadSound(const char *path);   // Load and store a sound
  void stopSounds();                  // Stop all sounds
  void unloadSound(const char *path); // Unload a sound
  void unloadSounds();                // Unload all sounds
  void preloadSounds(
      const std::vector<std::string> &paths); // Preload multiple sounds
  bool playSound(const char *path);           // Play a preloaded sound

private:
  ma_engine engine;
  ma_engine_config engineConfig;
  // sound group
  ma_sound_group soundGroup;
  ma_sound_group_config soundGroupConfig;
  std::map<std::string, ma_sound> sounds;
};
