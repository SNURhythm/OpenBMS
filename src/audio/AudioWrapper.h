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

// Reverb Helper Structures
struct CombFilter {
  std::vector<float> buffer;
  size_t index = 0;
  float feedback = 0.8f;
  float damp = 0.2f;
  float filterStore = 0.0f;

  void init(size_t size);
  float process(float input);
};

struct AllPassFilter {
  std::vector<float> buffer;
  size_t index = 0;
  float feedback = 0.5f;

  void init(size_t size);
  float process(float input);
};

struct SimpleReverb {
  CombFilter combs[4];
  AllPassFilter allpasses[2];
  float wet = 0.0f; // 0.0 to 1.0 volume of wet signal
  bool initialized = false;

  void init(int sampleRate);
  void processStereo(float *buffer, size_t frameCount);
  void setMix(float mix); // 0.0 - 0.5 (subtle) to 1.0 (huge)
};

struct SimpleCompressor {
  float thresholdDb = 0.0f;
  float ratio = 1.0f;
  float attack = 0.01f;
  float release = 0.1f;
  float envelope = 0.0f;
  int sampleRate = 44100;
  bool enabled = false;

  void init(int rate);
  void processStereo(float *buffer, size_t frameCount);
  void setParams(float threshold, float ratio, float attack, float release);
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
  SimpleReverb *reverb;
  SimpleCompressor *compressor;
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
  void setReverbMix(float mix);
  void setCompressor(float threshold, float ratio);

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
  SimpleReverb reverb;
  SimpleCompressor compressor;
  int currentSampleRate = 44100;

  UserData userData;
  Stopwatch *stopwatch;
};
