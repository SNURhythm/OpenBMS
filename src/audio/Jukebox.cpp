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
void Jukebox::render(){
  if(currentBga != -1){
    if(videoPlayerTable.find(currentBga) != videoPlayerTable.end()){
      auto videoPlayer = videoPlayerTable[currentBga];
      videoPlayer->update();
      videoPlayer->render();
    }
  }
}
void Jukebox::loadSounds(bms_parser::Chart &chart,
                         std::atomic_bool &isCancelled){
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
}
void Jukebox::loadBMPs(bms_parser::Chart &chart, std::atomic_bool &isCancelled) {
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
        auto fileName = pathHash+"-" + std::to_string(bmp->first) + ".mp4";
        auto transcodedPath = (Utils::GetDocumentsPath("temp")/fileName).string();
        if(!std::filesystem::exists(transcodedPath)) {
          // mkdir
          std::filesystem::create_directories(Utils::GetDocumentsPath("temp"));
          int result = transcode(path.string().c_str(), transcodedPath.c_str(),
                                 &isCancelled);
          if (result != 0) {
            SDL_Log("Failed to transcode video: %ls", path.c_str());
            continue;
          }
        }
        // new video player
        auto videoPlayer = new VideoPlayer();
        if (videoPlayer->loadVideo(transcodedPath)) {
          videoPlayerTable[bmp->first] = videoPlayer;

          SDL_Log("video width: %f, video height: %f", videoPlayer->viewWidth, videoPlayer->viewHeight);

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
}
void Jukebox::loadChart(bms_parser::Chart &chart,
                        std::atomic_bool &isCancelled) {
  isPlaying = false;
  if (playThread.joinable())
    playThread.join();
  audioQueue = std::queue<std::pair<long long, int>>();
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
  loadSounds(chart, isCancelled);
  loadBMPs(chart, isCancelled);

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
      if(timeline->BgaBase != -1){
        bmpQueue.emplace(timeline->Timing, timeline->BgaBase);
      }
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
        audioQueue.push(note);
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
    while (isPlaying) {
      auto now = std::chrono::high_resolution_clock::now();
      auto position = now - startPos;
      auto positionMicro =
          std::chrono::duration_cast<std::chrono::microseconds>(position)
              .count();
      if(!audioQueue.empty()) {
        if (positionMicro >= audioQueue.front().first) {
          SDL_Log("Playing sound at %lld; id: %d; actual time: %lld",
                  audioQueue.front().first, audioQueue.front().second,
                  positionMicro);
          audio.playSound(wavTableAbs[audioQueue.front().second].c_str());
          audioQueue.pop();
        }
      }
      if(!bmpQueue.empty()) {
        if (positionMicro >= bmpQueue.front().first) {
          SDL_Log("Playing video at %lld; id: %d; actual time: %lld",
                  bmpQueue.front().first, bmpQueue.front().second,
                  positionMicro);
          if(videoPlayerTable.find(bmpQueue.front().second) != videoPlayerTable.end()){
            auto videoPlayer = videoPlayerTable[bmpQueue.front().second];
            videoPlayer->play();
            videoPlayer->viewWidth = rendering::window_width;
            videoPlayer->viewHeight = rendering::window_height;
            currentBga = bmpQueue.front().second;
          } else {
            SDL_Log("Video player not found for id: %d", bmpQueue.front().second);
          }
          bmpQueue.pop();
        }
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