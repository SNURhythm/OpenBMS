#define MINIAUDIO_IMPLEMENTATION
#include "AudioWrapper.h"
#include <stdexcept>
#include <SDL2/SDL.h>
#include "decoder.h"
#include <sndfile.h>
#include <stdio.h>
#include <mutex>

void dataCallback(ma_device *pDevice, void *pOutput, const void *pInput,
                  ma_uint32 frameCount) {
  auto *userData = (UserData *)pDevice->pUserData;
  if (userData == nullptr) {
    return;
  }
  std::lock_guard<std::mutex> lock(*userData->mutex);
  auto *soundDataList = userData->soundDataList;
  if (soundDataList == nullptr) {
    return;
  }
  if (soundDataList->empty())
    return;

  memset(pOutput, 0,
         frameCount * sizeof(ma_int16) * pDevice->playback.channels);

  float gain = 0.9f;

  for (auto &soundData : *soundDataList) {
    if (!soundData->playing) {
      continue;
    }

    ma_uint32 framesToRead = frameCount;
    ma_uint32 framesAvailable =
        soundData->resampledFrameCount - soundData->currentFrame;
    if (framesToRead > framesAvailable) {
      framesToRead = framesAvailable;
    }

    for (ma_uint32 frame = 0; frame < framesToRead; ++frame) {
      for (int channel = 0; channel < soundData->channels; ++channel) {
        int outputChannel = channel % pDevice->playback.channels;
        ma_int32 sample =
            ((ma_int16 *)
                 pOutput)[frame * pDevice->playback.channels + outputChannel] +
            static_cast<ma_int32>(
                gain *
                soundData->resampledData[(soundData->currentFrame + frame) *
                                             soundData->channels +
                                         channel]);
        // Clamp the sample to the range of ma_int16
        if (sample > 32767)
          sample = 32767;
        if (sample < -32768)
          sample = -32768;
        ((ma_int16 *)
             pOutput)[frame * pDevice->playback.channels + outputChannel] =
            sample;
      }
    }

    soundData->currentFrame += framesToRead;
    if (framesToRead < frameCount) {
      soundData->playing = false; // Stop the sound if all frames have been read
    }
  }
}
AudioWrapper::AudioWrapper() {
  userData.mutex = &soundDataListMutex;
  userData.soundDataList = &soundDataList;
  ma_device_config deviceConfig =
      ma_device_config_init(ma_device_type_playback);
  deviceConfig.playback.format = ma_format_s16;
  deviceConfig.playback.channels = 2; // Assuming stereo output
  deviceConfig.sampleRate = 44100;    // Assuming 44100 Hz sample rate
  deviceConfig.dataCallback = dataCallback;
  deviceConfig.pUserData = &userData;

  auto result = ma_device_init(nullptr, &deviceConfig, &device);
  if (result != MA_SUCCESS) {
    throw std::runtime_error("Failed to initialize playback device.");
  }

  ma_device_start(&device);
}

AudioWrapper::~AudioWrapper() { unloadSounds(); }

bool AudioWrapper::loadSound(const path_t &path) {
  SF_INFO sfInfo;
  auto soundData = std::make_shared<SoundData>();
  bool result = decodeAudioToPCM(path, soundData->pcmData, sfInfo);
  if (!result) {
    SDL_Log("Failed to decode audio file %ls", path.c_str());
    return false;
  }

  soundData->currentFrame = 0;
  soundData->channels = sfInfo.channels;
  soundData->originalSampleRate = sfInfo.samplerate;
  soundData->playing = false;

  // Initialize the resampler
  ma_resampler_config resamplerConfig = ma_resampler_config_init(
      ma_format_s16, sfInfo.channels, sfInfo.samplerate, 44100,
      ma_resample_algorithm_linear);
  if (ma_resampler_init(&resamplerConfig, nullptr, &soundData->resampler) !=
      MA_SUCCESS) {
    SDL_Log("Failed to initialize resampler.");
    return false;
  }

  // Resample the audio data to 44100 Hz

  ma_uint64 resampledFrameCount =
      (ma_uint64)((double)soundData->pcmData.size() / sfInfo.channels * 44100 /
                  sfInfo.samplerate);
  soundData->resampledData.resize(resampledFrameCount * sfInfo.channels);
  ma_uint64 size = (ma_uint64)soundData->pcmData.size();
  ma_resampler_process_pcm_frames(
      &soundData->resampler, soundData->pcmData.data(), &size,
      soundData->resampledData.data(), &resampledFrameCount);
  soundData->resampledFrameCount = resampledFrameCount;
  {
    std::lock_guard<std::mutex> lock(soundDataListMutex);
    soundDataIndexMap[path] = soundDataList.size();
    soundDataList.push_back(soundData);
  }
  return true;
}

void AudioWrapper::preloadSounds(const std::vector<path_t> &paths) {
  for (const auto &path : paths) {
    loadSound(path);
  }
}

bool AudioWrapper::playSound(const path_t &path) {
  std::lock_guard<std::mutex> lock(soundDataListMutex);
  if (!ma_device_is_started(&device)) {
    ma_device_start(&device);
    SDL_Log("Started playback device.");
  }
  if (soundDataIndexMap.find(path) == soundDataIndexMap.end()) {
    if (!loadSound(path)) {
      return false;
    }
  }

  auto &soundData = soundDataList[soundDataIndexMap[path]];
  soundData->currentFrame = 0;
  soundData->playing = true;

  return true;
}
void AudioWrapper::startDevice() {
  if (!ma_device_is_started(&device)) {
    ma_device_start(&device);
    SDL_Log("Started playback device.");
  }
}
void AudioWrapper::stopSounds() {
  std::lock_guard<std::mutex> lock(soundDataListMutex);
  ma_device_stop(&device);
  for (auto &soundData : soundDataList) {
    soundData->playing = false;
  }
}

void AudioWrapper::unloadSound(const path_t &path) {
  std::lock_guard<std::mutex> lock(soundDataListMutex);
  if (soundDataIndexMap.find(path) != soundDataIndexMap.end()) {
    size_t index = soundDataIndexMap[path];
    ma_resampler_uninit(&soundDataList[index]->resampler,
                        nullptr); // Cleanup resampler
    soundDataList.erase(soundDataList.begin() + index);
    soundDataIndexMap.erase(path);

    // Update indices in the map
    for (auto &entry : soundDataIndexMap) {
      if (entry.second > index) {
        entry.second--;
      }
    }
  }
}

void AudioWrapper::unloadSounds() {
  stopSounds();
  {
    std::lock_guard<std::mutex> lock(soundDataListMutex);
    for (auto &soundData : soundDataList) {
      ma_resampler_uninit(&soundData->resampler, nullptr); // Cleanup resampler
    }
    soundDataList.clear();
    soundDataIndexMap.clear();
  }
}
