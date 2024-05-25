
#define MINIAUDIO_IMPLEMENTATION
#include "AudioWrapper.h"
#include <stdexcept>
#include <SDL2/SDL.h>

#include <stdio.h>

AudioWrapper::AudioWrapper() {
  engineConfig = ma_engine_config_init();
  auto result = ma_engine_init(&engineConfig, &engine);
  if (result != MA_SUCCESS) {
    throw std::runtime_error("Failed to initialize audio engine.");
  }
  result = ma_sound_group_init(&engine, 0, nullptr, &soundGroup);
}

AudioWrapper::~AudioWrapper() {
  unloadSounds();

  ma_engine_uninit(&engine);
}

bool AudioWrapper::loadSound(const char *path) {
  ma_sound_uninit(&sounds[path]);
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
  auto playResult = ma_engine_play_sound(&engine, path, &soundGroup);
  SDL_Log("Playing sound %s, result %d", path, playResult);
  return result;
}

void AudioWrapper::stopSounds() {
  ma_sound_group_stop(&soundGroup);
  ma_sound_group_start(&soundGroup);
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
  ma_sound_group_uninit(&soundGroup);
  ma_engine_uninit(&engine);
  ma_engine_init(&engineConfig, &engine);
  ma_sound_group_init(&engine, 0, nullptr, &soundGroup);
}
