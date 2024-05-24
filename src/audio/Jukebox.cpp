#include "Jukebox.h"
#include <SDL2/SDL.h>
#include <thread>
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
  for (auto &wav : chart.WavTable) {
    if (isCancelled)
      return;
    bool loaded = false;
    std::filesystem::path basePath = chart.Meta.Folder / wav.second;
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
        wavTableAbs[wav.first] = path;
        loaded = true;
        break;
      }
    }

    if (!loaded) {
      SDL_Log("Failed to load sound for all extensions: %s", basePath.c_str());
    }
  }
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
  playThread = std::thread([this] {
    while (isPlaying && !scheduleQueue.empty()) {
      auto now = std::chrono::high_resolution_clock::now();
      auto position = now - startPos;
      auto positionMicro =
          std::chrono::duration_cast<std::chrono::microseconds>(position)
              .count();
      if (positionMicro >= scheduleQueue.front().first) {
        SDL_Log("Playing sound: %s",
                wavTableAbs[scheduleQueue.front().second].c_str());
        audio.playSound(wavTableAbs[scheduleQueue.front().second].c_str());
        scheduleQueue.pop();
      }
    }
  });
}