#pragma once

#include <miniaudio.h>
#include <map>
#include <string>
#include <vector>
#include "../path.h"
// Custom data structure to hold PCM data and playback state
struct SoundData {
  std::vector<short> pcmData;
  size_t currentFrame;
  int channels;
  int originalSampleRate;
  bool playing;
  ma_resampler resampler;
  std::vector<short> resampledData;
  size_t resampledFrameCount;
};
class AudioWrapper {
public:
  AudioWrapper();
  ~AudioWrapper();
  bool loadSound(const path_t &path);
  void preloadSounds(const std::vector<path_t> &paths);
  bool playSound(const path_t &path);
  void stopSounds();
  void unloadSound(const path_t &path);
  void unloadSounds();

private:
  ma_engine engine;
  ma_engine_config engineConfig;
  ma_device device;
  std::vector<std::shared_ptr<SoundData>> soundDataList;
  std::map<path_t, size_t>
      soundDataIndexMap; // Map to store index of SoundData in soundDataList
};
