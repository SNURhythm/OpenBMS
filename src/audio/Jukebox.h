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
#include "../utils/Stopwatch.h"

class Jukebox {
public:
  Jukebox();
  ~Jukebox();

  void loadChart(bms_parser::Chart &chart, bool scheduleNotes,
                 std::atomic_bool &isCancelled);
  void schedule(bms_parser::Chart &chart, bool scheduleNotes,
                std::atomic_bool &isCancelled);
  void playKeySound(int wav);
  void play();
  void stop();
  void render();

  long long getTimeMicros();
  void seek(long long micro);
  std::function<void(long long)> onTickCb;
  void onTick(const std::function<void(long long)> &cb) { onTickCb = cb; }
  void removeOnTick() { onTickCb = nullptr; }

private:
  // seek lock
  std::mutex seekLock;
  void loadSounds(bms_parser::Chart &chart, std::atomic_bool &isCancelled);
  void loadBMPs(bms_parser::Chart &chart, std::atomic_bool &isCancelled);
  std::atomic_bool isPlaying = false;
  std::thread playThread;
  Stopwatch stopwatch;
  AudioWrapper audio;
  std::vector<std::pair<long long, int>> audioList;
  size_t audioCursor = 0;
  std::vector<std::pair<long long, int>> bmpList;
  size_t bmpCursor = 0;
  std::unordered_map<int, path_t> wavTableAbs;
  std::unordered_map<int, VideoPlayer *> videoPlayerTable;
  std::vector<std::future<bool>> asyncVideoLoads;
  int currentBga = -1;
  std::vector<std::string> audioExtensions = {"flac", "wav", "ogg", "mp3"};
  std::vector<std::string> videoExtensions = {
      "mp4", "wmv", "m4v", "webm", "mpg", "mpeg", "m1v", "m2v", "avi"};
};
