#pragma once

#include <miniaudio.h>
#include <map>
#include <string>
#include <vector>
#include "../path.h"
#include "../utils/Stopwatch.h"
#include <mutex>
#include <atomic>
// Custom data structure to hold PCM data and playback state
struct SoundData {

  size_t currentFrame;
  int channels;
  int originalSampleRate;
  bool playing;
  ma_resampler resampler;
  std::vector<short> resampledData;
  size_t resampledFrameCount;
};
struct UserData {
  Stopwatch *stopwatch;
  std::mutex *mutex;
  std::vector<std::shared_ptr<SoundData>> *soundDataList;
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

  void unloadSounds();

  struct IAudioBackend; // Forward declaration

private:
  std::unique_ptr<IAudioBackend> backend;

  std::vector<std::shared_ptr<SoundData>> soundDataList;
  std::map<path_t, size_t>
      soundDataIndexMap; // Map to store index of SoundData in soundDataList
  std::mutex soundDataListMutex;
  UserData userData;
  Stopwatch *stopwatch;
};
