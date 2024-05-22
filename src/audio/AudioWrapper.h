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
  void unloadSound(const char *path); // Unload a sound
  void unloadSounds();                // Unload all sounds
  void preloadSounds(
      const std::vector<std::string> &paths); // Preload multiple sounds
  bool playSound(const char *path);           // Play a preloaded sound

private:
  ma_engine engine;
  std::map<std::string, ma_sound> sounds;
};