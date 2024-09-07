#include "Jukebox.h"
#include <SDL2/SDL.h>
#include <thread>
#include "../Utils.h"
#include "../video/transcode.h"
#include "../rendering/common.h"
Jukebox::Jukebox() {}

Jukebox::~Jukebox() {
  isPlaying = false;
  if (playThread.joinable())
    playThread.join();
  audio.stopSounds();
  audio.unloadSounds();
  for (auto &videoPlayer : videoPlayerTable) {
    delete videoPlayer.second;
  }
}
void Jukebox::render() {
  if (currentBga != -1) {
    if (videoPlayerTable.find(currentBga) != videoPlayerTable.end()) {
      auto videoPlayer = videoPlayerTable[currentBga];
      videoPlayer->viewWidth = rendering::window_width;
      videoPlayer->viewHeight = rendering::window_height;
      videoPlayer->update();
      videoPlayer->render();
    }
  }
}

void Jukebox::loadSounds(bms_parser::Chart &chart,
                         std::atomic_bool &isCancelled) {
  parallel_for(chart.WavTable.size(), [&](int start, int end) {
    auto wav = std::next(chart.WavTable.begin(), start);
    for (int i = start; i < end; i++, ++wav) {
      if (isCancelled)
        return;
      bool found = false;
      std::filesystem::path basePath = chart.Meta.Folder / wav->second;
      std::filesystem::path path;

      // Try each extension until one succeeds
      for (const auto &ext : audioExtensions) {
        if (isCancelled)
          return;
        path = basePath;
        path.replace_extension(ext);
        if (!std::filesystem::exists(path)) {
          continue;
        }
        if (audio.loadSound(path.c_str(), isCancelled)) {
          wavTableAbs[wav->first] = path;
          found = true;
          break;
        }
      }
      if (!found) {
        SDL_Log("Failed to load sound for all extensions: %s",
                basePath.c_str());
      }
    }
  });
}
void Jukebox::loadBMPs(bms_parser::Chart &chart,
                       std::atomic_bool &isCancelled) {
  parallel_for(chart.BmpTable.size(), [&](int start, int end) {
    auto bmp = std::next(chart.BmpTable.begin(), start);
    for (int i = start; i < end; i++, ++bmp) {
      if (isCancelled)
        return;
      bool found = false;
      std::filesystem::path basePath = chart.Meta.Folder / bmp->second;
      std::filesystem::path path;

      // Try each extension until one succeeds
      for (const auto &ext : videoExtensions) {
        if (isCancelled)
          return;
        path = basePath;
        path.replace_extension(ext);
        if (!std::filesystem::exists(path)) {
          continue;
        }
        // calculate hash of base path
        auto pathHash = bms_parser::md5(basePath.string());
        auto fileName = pathHash + "-" + std::to_string(bmp->first) + ".mp4";
        auto transcodedPath =
            (Utils::GetDocumentsPath("temp") / fileName).string();
        if (!std::filesystem::exists(transcodedPath)) {
          // mkdir
          std::filesystem::create_directories(Utils::GetDocumentsPath("temp"));
          int result = transcode(path.string().c_str(), transcodedPath.c_str(),
                                 &isCancelled);
          if (isCancelled) {
            // delete transcoded file
            std::filesystem::remove(transcodedPath);
            return;
          }
          if (result != 0) {
            SDL_Log("Failed to transcode video: %ls", path.c_str());
            continue;
          }
        }
        // new video player
        auto videoPlayer = new VideoPlayer();
        if (videoPlayer->loadVideo(transcodedPath, isCancelled)) {
          videoPlayerTable[bmp->first] = videoPlayer;

          SDL_Log("video width: %f, video height: %f", videoPlayer->viewWidth,
                  videoPlayer->viewHeight);

          found = true;
          SDL_Log("Loaded video to id: %d", bmp->first);
          break;
        } else {
          SDL_Log("Failed to load video: %s", path.c_str());
          delete videoPlayer;
        }
      }

      // if not found, fall back to image loading
      if (!found) {
        // TODO: implement
      }
    }
  });
}
void Jukebox::loadChart(bms_parser::Chart &chart, bool scheduleNotes,
                        std::atomic_bool &isCancelled) {
  isPlaying = false;
  if (playThread.joinable())
    playThread.join();

  audio.stopSounds();
  audio.unloadSounds();
  wavTableAbs.clear();
  currentBga = -1;
  for (auto &videoPlayer : videoPlayerTable) {
    delete videoPlayer.second;
  }
  videoPlayerTable.clear();

  if (isCancelled)
    return;
  SDL_Log("Loading sounds");
  std::thread loadSoundThread(
      [this, &chart, &isCancelled] { loadSounds(chart, isCancelled); });
  SDL_Log("Loading videos");
  loadBMPs(chart, isCancelled);
  loadSoundThread.join();

  if (isCancelled)
    return;
  schedule(chart, scheduleNotes, isCancelled);
  SDL_Log("Chart loaded");
}

void Jukebox::schedule(bms_parser::Chart &chart, bool scheduleNotes,
                       std::atomic_bool &isCancelled) {
  audioCursor = 0;
  bmpCursor = 0;
  audioList.clear();
  bmpList.clear();
  for (auto &measure : chart.Measures) {
    if (isCancelled)
      return;
    for (auto &timeline : measure->TimeLines) {
      if (isCancelled)
        return;
      if (timeline->BgaBase != -1) {
        BMPData data{
            .id = timeline->BgaBase,
            .viewId = rendering::bga_view,
        };

        bmpList.emplace_back(timeline->Timing, data);
      }
      if (timeline->BgaLayer != -1) {
        BMPData data{
            .id = timeline->BgaLayer,
            .viewId = rendering::bga_layer_view,
        };
        bmpList.emplace_back(timeline->Timing, data);
      }
      std::vector<std::pair<long long, int>> notes;
      if (scheduleNotes) {
        for (auto &note : timeline->Notes) {
          if (isCancelled)
            return;
          if (note == nullptr)
            continue;
          notes.emplace_back(timeline->Timing, note->Wav);
        }
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
        audioList.push_back(note);
      }
    }
  }
}
void Jukebox::playKeySound(int wav) {
  if (isPlaying) {
    audio.playSound(wavTableAbs[wav].c_str());
  }
}

void Jukebox::play() {
  if (playThread.joinable())
    playThread.join();
  audio.startDevice();
  isPlaying = true;
  stopwatch.reset();
  stopwatch.start();
  auto hz = 8000;
  playThread = std::thread([this, hz] {
    while (isPlaying) {
      // lock seek
      std::lock_guard<std::mutex> lock(seekLock);
      auto now = std::chrono::high_resolution_clock::now();
      auto positionMicro = stopwatch.elapsedMicros();
      if (onTickCb) {
        onTickCb(positionMicro);
      }

      if (audioCursor < audioList.size()) {
        auto &target = audioList[audioCursor];
        if (positionMicro >= target.first) {
          //          SDL_Log("Playing sound at %lld; id: %d; actual time:
          //          %lld",
          //                  target.first, target.second, positionMicro);
          audio.playSound(wavTableAbs[target.second].c_str());
          audioCursor++;
        }
      }
      if (bmpCursor < bmpList.size()) {
        auto &target = bmpList[bmpCursor];
        if (positionMicro >= target.first) {
          //          SDL_Log("Playing video at %lld; id: %d; actual time:
          //          %lld",
          //                  target.first, target.second, positionMicro);
          if (videoPlayerTable.find(target.second.id) !=
              videoPlayerTable.end()) {
            auto videoPlayer = videoPlayerTable[target.second.id];
            videoPlayer->seek(0);
            videoPlayer->play();
            videoPlayer->viewWidth = rendering::window_width;
            videoPlayer->viewHeight = rendering::window_height;
            videoPlayer->viewId = target.second.viewId;
            currentBga = target.second.id;
          } else {
            SDL_Log("Video player not found for id: %d", target.second.id);
          }
          bmpCursor++;
        }
      }
      auto loopRunTime = std::chrono::high_resolution_clock::now() - now;
      auto sleepTime = std::chrono::microseconds(1000000 / hz) - loopRunTime;
      std::this_thread::sleep_for(sleepTime);
    }
  });
}

long long Jukebox::getTimeMicros() { return stopwatch.elapsedMicros(); }

void Jukebox::stop() {
  isPlaying = false;
  if (playThread.joinable())
    playThread.join();
  audio.stopSounds();
}
void Jukebox::seek(long long micro) {
  /* TODO: should also play audio/video which starts earlier than seek but
      ends later than seek */
  std::lock_guard<std::mutex> lock(seekLock);
  stopwatch.seek(micro);
  audio.stopSounds();
  // move cursors to micro
  audioCursor = 0;
  bmpCursor = 0;
  while (audioCursor < audioList.size() &&
         audioList[audioCursor].first < micro) {
    audioCursor++;
  }
  while (bmpCursor < bmpList.size() && bmpList[bmpCursor].first < micro) {
    bmpCursor++;
  }
}
