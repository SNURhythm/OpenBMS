#pragma once

#include <miniaudio.h>
#include <map>
#include <string>
#include <vector>
#include "../path.h"
#include "../utils/Stopwatch.h"
#include <mutex>
#include <atomic>
#include <atomic>
#include <cmath>

// Simple Biquad Filter
struct Biquad {
  float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
  float z1 = 0.0f, z2 = 0.0f;
  float z1_r = 0.0f, z2_r = 0.0f; // For stereo (right channel)

  void processStereo(float *buffer, size_t frameCount);
  void setLowShelf(float fs, float f0, float gainDb, float Q = 0.707f);
  void setHighShelf(float fs, float f0, float gainDb, float Q = 0.707f);
};

// Custom data structure to hold PCM data and playback state
struct SoundData {

  size_t currentFrame;
  int channels;
  int originalSampleRate;
  bool playing;
  bool isResampled;
  ma_resampler resampler;
  std::vector<short> resampledData;
  size_t resampledFrameCount;
};
struct UserData {
  Stopwatch *stopwatch;
  std::mutex *mutex;
  std::vector<std::shared_ptr<SoundData>> *soundDataList;
  std::vector<float> *mixBuffer;
  Biquad *bassFilter;
  Biquad *trebleFilter;
};
class AudioWrapper {
public:
  AudioWrapper(Stopwatch *stopwatch);
  ~AudioWrapper();
  bool loadSound(const path_t &path, std::atomic<bool> &isCancelled);
  void preloadSounds(const std::vector<path_t> &paths,
                     std::atomic<bool> &isCancelled);
  bool playSound(const path_t &path);
  void startDevice();
  void stopSounds();
  void unloadSound(const path_t &path);

  void setBassBoost(float db);
  void setTrebleBoost(float db);

  void unloadSounds();

  struct IAudioBackend; // Forward declaration

private:
  std::unique_ptr<IAudioBackend> backend;

  std::vector<std::shared_ptr<SoundData>> soundDataList;
  std::map<path_t, size_t>
      soundDataIndexMap; // Map to store index of SoundData in soundDataList
  std::mutex soundDataListMutex;
  std::vector<float> mixBuffer;
  Biquad bassFilter;
  Biquad trebleFilter;
  int currentSampleRate = 44100;

  UserData userData;
  Stopwatch *stopwatch;
};
