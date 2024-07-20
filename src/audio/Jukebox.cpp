#include "Jukebox.h"
#include <SDL2/SDL.h>
#include <thread>
#include "../Utils.h"
Jukebox::Jukebox() {}

Jukebox::~Jukebox() {
  isPlaying = false;
  if (playThread.joinable())
    playThread.join();
}

void Jukebox::loadChart(bms_parser::Chart &chart,
                        std::atomic_bool &isCancelled) {
  isPlaying = false;
  if (playThread.joinable())
    playThread.join();
  scheduleQueue = std::queue<std::pair<long long, int>>();
  audio.stopSounds();
  audio.unloadSounds();
  if (isCancelled)
    return;
  parallel_for(chart.WavTable.size(), [&](int start, int end) {
    auto wav = std::next(chart.WavTable.begin(), start);
    for (int i = start; i < end; i++, ++wav) {
      if (isCancelled)
        return;
      bool found = false;
      std::filesystem::path basePath = chart.Meta.Folder / wav->second;
      std::filesystem::path path;

      // Try each extension until one succeeds
      for (const auto &ext : extensions) {
        if (isCancelled)
          return;
        path = basePath;
        path.replace_extension(ext);
        if (!std::filesystem::exists(path)) {
          continue;
        }
        if (audio.loadSound(path.c_str())) {
          wavTableAbs[wav->first] = path;
          found = true;
          break;
        }
      }
      if (!found) {
        SDL_Log("Failed to load sound for all extensions: %s", basePath.c_str());
      }
    }
  });
  std::vector<int> keys;
  for (auto &wav : wavTableAbs) {
    if (isCancelled)
      return;
    keys.push_back(wav.first);
  }

  parallel_for(keys.size(), [&](int start, int end) {
    for (int i = start; i < end; i++) {
      if (isCancelled)
        return;
      auto wav = wavTableAbs[keys[i]];
      bool loaded = false;
      std::filesystem::path path = wav;

      if (isCancelled)
        return;
      if (audio.loadSound(path.c_str())) {
        loaded = true;
        break;
      }

      if (!loaded) {
        SDL_Log("Failed to load sound for all extensions: %s", path.c_str());
      }
    }
  });

  if (isCancelled)
    return;
  schedule(chart, isCancelled);
}

void Jukebox::schedule(bms_parser::Chart &chart,
                       std::atomic_bool &isCancelled) {

  for (auto &measure : chart.Measures) {
    if (isCancelled)
      return;
    for (auto &timeline : measure->TimeLines) {
      if (isCancelled)
        return;
      std::vector<std::pair<long long, int>> notes;
      for (auto &note : timeline->Notes) {
        if (isCancelled)
          return;
        if (note == nullptr)
          continue;
        notes.emplace_back(timeline->Timing, note->Wav);
      }
      for (auto &bgNote : timeline->BackgroundNotes) {
        if (isCancelled)
          return;
        notes.emplace_back(timeline->Timing, bgNote->Wav);
      }
      std::sort(notes.begin(), notes.end());
      for (auto &note : notes) {
        if (isCancelled)
          return;
        scheduleQueue.push(note);
      }
    }
  }
}

void Jukebox::play() {
  if (playThread.joinable())
    playThread.join();
  isPlaying = true;
  startPos = std::chrono::high_resolution_clock::now();
  auto hz = 8000;
  playThread = std::thread([this, hz] {
    while (isPlaying && !scheduleQueue.empty()) {
      auto now = std::chrono::high_resolution_clock::now();
      auto position = now - startPos;
      auto positionMicro =
          std::chrono::duration_cast<std::chrono::microseconds>(position)
              .count();
      if (positionMicro >= scheduleQueue.front().first) {
        SDL_Log("Playing sound at %lld; id: %d; actual time: %lld", scheduleQueue.front().first,
                scheduleQueue.front().second, positionMicro);
        audio.playSound(wavTableAbs[scheduleQueue.front().second].c_str());
        scheduleQueue.pop();
      }
      auto loopRunTime = std::chrono::high_resolution_clock::now() - now;
      auto sleepTime = std::chrono::microseconds(1000000 / hz) - loopRunTime;
      std::this_thread::sleep_for(sleepTime);
    }
  });
}

void Jukebox::stop() {
  isPlaying = false;
  if (playThread.joinable())
    playThread.join();
  audio.stopSounds();
}