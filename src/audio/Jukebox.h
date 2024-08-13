#pragma once
#include "../bms_parser.hpp"
#include "AudioWrapper.h"
#include <chrono>
#include <queue>
#include <thread>
#include <unordered_map>
#include <atomic>
#include "../path.h"
#include "../video/VideoPlayer.h"
class Jukebox {
public:
  Jukebox();
  ~Jukebox();

  void loadChart(bms_parser::Chart &chart, std::atomic_bool &isCancelled);
  void schedule(bms_parser::Chart &chart, std::atomic_bool &isCancelled);
  void play();
  void stop();
  void render();

  long long getTimeMicros();

private:
  void loadSounds(bms_parser::Chart &chart, std::atomic_bool &isCancelled);
  void loadBMPs(bms_parser::Chart &chart, std::atomic_bool &isCancelled);
  std::atomic_bool isPlaying = false;
  std::thread playThread;
  std::chrono::high_resolution_clock::time_point startPos;
  std::chrono::high_resolution_clock::time_point position;
  AudioWrapper audio;
  std::queue<std::pair<long long, int>> audioQueue;
  std::queue<std::pair<long long, int>> bmpQueue;
  std::unordered_map<int, path_t> wavTableAbs;
  std::unordered_map<int, VideoPlayer*> videoPlayerTable;
  int currentBga = -1;
  std::vector<std::string> audioExtensions = {"flac", "wav", "ogg", "mp3"};
  std::vector<std::string> videoExtensions = { "mp4", "wmv", "m4v", "webm", "mpg", "mpeg", "m1v", "m2v", "avi"};
};
