
#define MINIAUDIO_IMPLEMENTATION
// NOTE: Should be compiled as Objective-C++ in iOS
// TODO: Find a way to automatically compile as Objective-C++ in iOS
#include "AudioWrapper.h"
#include <stdexcept>
#include <SDL2/SDL.h>
AudioWrapper::AudioWrapper() {
  auto result = ma_engine_init(nullptr, &engine);
  if (result != MA_SUCCESS) {
    throw std::runtime_error("Failed to initialize audio engine.");
  }
}

AudioWrapper::~AudioWrapper() {
  ma_engine_uninit(&engine);
  unloadSounds();
}

bool AudioWrapper::loadSound(const char *path) {
  SDL_Log("Loading sound: %s", path);
  auto result = ma_sound_init_from_file(&engine, path, MA_SOUND_FLAG_DECODE,
                                        nullptr, nullptr, &sounds[path]);
  return result == MA_SUCCESS;
}

void AudioWrapper::preloadSounds(const std::vector<std::string> &paths) {
  for (const auto &path : paths) {
    loadSound(path.c_str());
  }
}

bool AudioWrapper::playSound(const char *path) {
  bool result = true;
  if (sounds.find(path) == sounds.end()) {
    result = loadSound(path);
  }
  ma_engine_play_sound(&engine, path, nullptr);
  return result;
}

void AudioWrapper::unloadSound(const char *path) {
  ma_sound sound = sounds[path];
  ma_sound_uninit(&sound);
  sounds.erase(path);
}

void AudioWrapper::unloadSounds() {
  for (auto &sound : sounds) {
    ma_sound_uninit(&sound.second);
  }
  sounds.clear();
}