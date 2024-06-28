
#define MINIAUDIO_IMPLEMENTATION
#include "AudioWrapper.h"
#include <stdexcept>
#include <SDL2/SDL.h>
#include "decoder.h"
#include "sndfile.h"
#include <stdio.h>
// Custom data structure to hold PCM data and playback state
struct SoundData {
  std::vector<short> pcmData;
  size_t currentFrame;
  int channels;
};
void dataCallback(ma_device *pDevice, void *pOutput, const void *pInput,
                  ma_uint32 frameCount) {
  auto *soundData = (SoundData *)pDevice->pUserData;
  if (soundData == nullptr) {
    return;
  }

  ma_uint32 framesToRead = frameCount;
  ma_uint32 framesAvailable =
      (soundData->pcmData.size() / soundData->channels) -
      soundData->currentFrame;
  if (framesToRead > framesAvailable) {
    framesToRead = framesAvailable;
  }

  ma_copy_pcm_frames(
      pOutput,
      &soundData->pcmData[soundData->currentFrame * soundData->channels],
      framesToRead, ma_format_s16, soundData->channels);

  soundData->currentFrame += framesToRead;

  // If we've reached the end of the PCM data, loop back to the start
  if (framesToRead < frameCount) {
    ma_copy_pcm_frames((ma_int16 *)pOutput + framesToRead * soundData->channels,
                       soundData->pcmData.data(), frameCount - framesToRead,
                       ma_format_s16, soundData->channels);
    soundData->currentFrame = frameCount - framesToRead;
  }
}
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

bool AudioWrapper::loadSound(const path_t &path) {
  // Uninitialize any existing sound
  ma_device_uninit(&devices[path]);

  // Read the PCM data
  SF_INFO sfInfo;
  std::vector<short> pcmData = decodeAudioToPCM(path, sfInfo);

  // Store the PCM data in a SoundData structure
  SoundData *soundData = new SoundData{pcmData, 0, sfInfo.channels};

  // Initialize the sound with the custom data callback
  ma_device_config deviceConfig =
      ma_device_config_init(ma_device_type_playback);
  deviceConfig.playback.format = ma_format_s16;
  deviceConfig.playback.channels = sfInfo.channels;
  deviceConfig.sampleRate = sfInfo.samplerate;
  deviceConfig.dataCallback = dataCallback;
  deviceConfig.pUserData = soundData;
  ma_device device;
  auto result = ma_device_init(nullptr, &deviceConfig, &device);
  if (result != MA_SUCCESS) {
    SDL_Log("Failed to initialize device for sound %s", path.c_str());
    return false;
  }
  devices[path] = device;
  return true;
}

void AudioWrapper::preloadSounds(const std::vector<path_t> &paths) {
  for (const auto &path : paths) {
    loadSound(path.c_str());
  }
}

bool AudioWrapper::playSound(const path_t &path) {
  bool result = true;
  if (devices.find(path) == devices.end()) {
    result = loadSound(path);
  }
  auto playResult =
      ma_device_start(&devices[path]); // Start playback of the sound
  SDL_Log("Playing sound %s, result %d", path.c_str(), playResult);
  return result;
}

void AudioWrapper::stopSounds() {
  ma_sound_group_stop(&soundGroup);
  ma_sound_group_start(&soundGroup);
}

void AudioWrapper::unloadSound(const path_t &path) {
  ma_device device = devices[path];
  ma_device_uninit(&device);
  devices.erase(path);
}

void AudioWrapper::unloadSounds() {
  for (auto &device : devices) {
    ma_device_uninit(&device.second);
  }
  devices.clear();
  ma_sound_group_uninit(&soundGroup);
  ma_engine_uninit(&engine);
  ma_engine_init(&engineConfig, &engine);
  ma_sound_group_init(&engine, 0, nullptr, &soundGroup);
}
