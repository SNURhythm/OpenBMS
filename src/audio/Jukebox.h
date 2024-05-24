#pragma once
#include "../bms_parser.hpp"
#include "AudioWrapper.h"
#include <chrono>
#include <queue>
#include <thread>
#include <unordered_map>
#include <atomic>

class Jukebox {
public:
  Jukebox();
  ~Jukebox();

  void loadChart(bms_parser::Chart &chart, std::atomic_bool &isCancelled);
  void schedule(bms_parser::Chart &chart, std::atomic_bool &isCancelled);
  void play();

  long long getTimeMicros();

private:
  std::atomic_bool isPlaying = false;
  std::thread playThread;
  std::chrono::high_resolution_clock::time_point startPos;
  std::chrono::high_resolution_clock::time_point position;
  AudioWrapper audio;
  std::queue<std::pair<long long, int>> scheduleQueue;
  std::unordered_map<int, std::string> wavTableAbs;
  std::vector<std::string> extensions = {"flac", "wav", "ogg", "mp3"};
};
