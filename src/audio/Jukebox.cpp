#include "Jukebox.h"
#include <SDL2/SDL.h>
#include <thread>
#include "../Utils.h"
#include "../game/GameState.h"
#include "../rendering/common.h"
#include "../rendering/ShaderManager.h"
#include "bgfx/bgfx.h"
#include <stb_image.h>
Jukebox::Jukebox(Stopwatch *stopwatch)
    : audio(stopwatch), stopwatch(stopwatch) {
  s_texColor = bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);
}

Jukebox::~Jukebox() {
  bgfx::destroy(s_texColor);
  isPlaying = false;
  if (playThread.joinable())
    playThread.join();
  audio.stopSounds();
  audio.unloadSounds();
  for (auto &videoPlayer : videoPlayerTable) {
    delete videoPlayer.second;
  }
  for (auto &image : imageTable) {
    bgfx::destroy(image.second.texture);
  }
}
void Jukebox::render() {
  if (currentBga != -1) {
    if (videoPlayerTable.find(currentBga) != videoPlayerTable.end()) {
      auto videoPlayer = videoPlayerTable[currentBga];
      videoPlayer->viewWidth = rendering::window_width;
      videoPlayer->viewHeight = rendering::window_height;
      videoPlayer->viewId = rendering::bga_view;
      videoPlayer->update();
      videoPlayer->render();
    } else if (imageTable.find(currentBga) != imageTable.end()) {
      auto image = imageTable[currentBga];
      renderImage(image, rendering::bga_view);
    }
  }
  if (currentBmpLayer != -1) {
    if (videoPlayerTable.find(currentBmpLayer) != videoPlayerTable.end()) {
      auto videoPlayer = videoPlayerTable[currentBmpLayer];
      videoPlayer->viewWidth = rendering::window_width;
      videoPlayer->viewHeight = rendering::window_height;
      videoPlayer->viewId = rendering::bga_layer_view;
      videoPlayer->update();
      videoPlayer->render();
    } else if (imageTable.find(currentBmpLayer) != imageTable.end()) {
      auto image = imageTable[currentBmpLayer];
      renderImage(image, rendering::bga_layer_view);
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
        // auto pathHash = bms_parser::md5(basePath.string());
        // auto fileName = pathHash + "-" + std::to_string(bmp->first) + ".mp4";
        // auto transcodedPath =
        //     (Utils::GetDocumentsPath("temp") / fileName).string();
        // if (!std::filesystem::exists(transcodedPath)) {
        //   // mkdir
        //   std::filesystem::create_directories(Utils::GetDocumentsPath("temp"));
        //   int result = transcode(path.string().c_str(),
        //   transcodedPath.c_str(),
        //                          &isCancelled);
        //   if (isCancelled) {
        //     // delete transcoded file
        //     std::filesystem::remove(transcodedPath);
        //     return;
        //   }
        //   if (result != 0) {
        //     SDL_Log("Failed to transcode video: %ls", path.c_str());
        //     continue;
        //   }
        // }
        // new video player
        auto videoPlayer = new VideoPlayer(stopwatch);
        path_t p = fspath_to_path_t(path);

        if (videoPlayer->loadVideo(path_t_to_utf8(p), isCancelled)) {
          std::lock_guard<std::mutex> lock(videoPlayerTableMutex);
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
        for (const auto &ext : imageExtensions) {
          if (isCancelled)
            return;
          path = basePath;
          path.replace_extension(ext);
          if (!std::filesystem::exists(path)) {
            continue;
          }
          path_t p = fspath_to_path_t(path);
          std::string utf8Path = path_t_to_utf8(p);
          int width, height, channels;
          unsigned char *data =
              stbi_load(utf8Path.c_str(), &width, &height, &channels, 4);
          if (!data) {
            SDL_Log("Failed to load image: %s", utf8Path.c_str());
            continue;
          }
          SDL_Log("Loaded image: %s", utf8Path.c_str());
          {
            std::lock_guard<std::mutex> lock(imageTableMutex);
            imageTable[bmp->first] = {
                .texture = bgfx::createTexture2D(
                    width, height, false, 1, bgfx::TextureFormat::RGBA8,
                    BGFX_TEXTURE_NONE, bgfx::copy(data, width * height * 4)),
                .width = width,
                .height = height,
                .channels = channels,
            };
          }
          stbi_image_free(data);
          break;
        }
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

  for (auto &image : imageTable) {
    bgfx::destroy(image.second.texture);
  }
  imageTable.clear();
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
  bmpLayerCursor = 0;
  audioList.clear();
  bmpList.clear();
  bmpLayerList.clear();
  for (auto &measure : chart.Measures) {
    if (isCancelled)
      return;
    for (auto &timeline : measure->TimeLines) {
      if (isCancelled)
        return;
      if (timeline->BgaBase != -1) {
        bmpList.emplace_back(timeline->Timing, timeline->BgaBase);
      }
      if (timeline->BgaLayer != -1) {
        bmpLayerList.emplace_back(timeline->Timing, timeline->BgaLayer);
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
  stopwatch->reset();
  stopwatch->start();
  auto hz = 8000;
  playThread = std::thread([this, hz] {
    while (isPlaying) {
      if (!stopwatch->isRunning()) {
        std::this_thread::sleep_for(std::chrono::microseconds(1000000 / hz));
        continue;
      }
      // lock seek
      std::lock_guard<std::mutex> lock(seekLock);
      auto now = std::chrono::high_resolution_clock::now();
      auto positionMicro = stopwatch->elapsedMicros();
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
          if (videoPlayerTable.find(target.second) != videoPlayerTable.end()) {
            auto videoPlayer = videoPlayerTable[target.second];
            videoPlayer->seek(0);
            videoPlayer->play();
            videoPlayer->viewWidth = rendering::window_width;
            videoPlayer->viewHeight = rendering::window_height;
            videoPlayer->viewId = rendering::bga_view;
            currentBga = target.second;
          } else if (imageTable.find(target.second) != imageTable.end()) {
            currentBga = target.second;
          }
          bmpCursor++;
        }
      }
      if (bmpLayerCursor < bmpLayerList.size()) {
        auto &target = bmpLayerList[bmpLayerCursor];
        if (positionMicro >= target.first) {
          //          SDL_Log("Playing video at %lld; id: %d; actual time:
          //          %lld",
          //                  target.first, target.second, positionMicro);
          if (videoPlayerTable.find(target.second) != videoPlayerTable.end()) {
            auto videoPlayer = videoPlayerTable[target.second];
            videoPlayer->seek(0);
            videoPlayer->play();
            videoPlayer->viewWidth = rendering::window_width;
            videoPlayer->viewHeight = rendering::window_height;
            videoPlayer->viewId = rendering::bga_layer_view;
            currentBmpLayer = target.second;
          } else if (imageTable.find(target.second) != imageTable.end()) {
            currentBmpLayer = target.second;
          }
          bmpLayerCursor++;
        }
      }
      auto loopRunTime = std::chrono::high_resolution_clock::now() - now;
      auto sleepTime = std::chrono::microseconds(1000000 / hz) - loopRunTime;
      std::this_thread::sleep_for(sleepTime);
    }
  });
}
void Jukebox::renderImage(ImageData &image, int viewId) {

  if (!bgfx::isValid(image.texture)) {
    return;
  }
  bgfx::VertexLayout layout;
  layout.begin()
      .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
      .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
      .end();

  bgfx::TransientVertexBuffer tvb{};
  bgfx::TransientIndexBuffer tib{};

  bgfx::allocTransientVertexBuffer(&tvb, 4, layout);
  bgfx::allocTransientIndexBuffer(&tib, 6);
  auto *vertex = (rendering::PosTexCoord0Vertex *)tvb.data;
  // canvas extension (See "spread canvas" in
  // https://hitkey.nekokan.dyndns.info/cmds.htm#BMPXX-ADJUSTMENT)
  vertex[0].x = rendering::window_width / 2.0f - image.width / 2.0f;
  vertex[0].y = rendering::window_height / 2.0f - 256.0f / 2.0f + image.height;
  vertex[0].z = 0.0f;
  vertex[0].u = 0.0f;
  vertex[0].v = 1.0f;
  vertex[1].x = rendering::window_width / 2.0f + image.width / 2.0f;
  vertex[1].y = rendering::window_height / 2.0f - 256.0f / 2.0f + image.height;
  vertex[1].z = 0.0f;
  vertex[1].u = 1.0f;
  vertex[1].v = 1.0f;
  vertex[2].x = rendering::window_width / 2.0f - image.width / 2.0f;
  vertex[2].y = rendering::window_height / 2.0f - 256.0f / 2.0f;
  vertex[2].z = 0.0f;
  vertex[2].u = 0.0f;
  vertex[2].v = 0.0f;
  vertex[3].x = rendering::window_width / 2.0f + image.width / 2.0f;
  vertex[3].y = rendering::window_height / 2.0f - 256.0f / 2.0f;
  vertex[3].z = 0.0f;
  vertex[3].u = 1.0f;
  vertex[3].v = 0.0f;
  auto *indices = (uint16_t *)tib.data;
  indices[0] = 0;
  indices[1] = 1;
  indices[2] = 2;
  indices[3] = 1;
  indices[4] = 3;
  indices[5] = 2;
  bgfx::setVertexBuffer(0, &tvb);
  bgfx::setIndexBuffer(&tib);
  bgfx::setTexture(0, s_texColor, image.texture);
  bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
                 BGFX_STATE_BLEND_ALPHA);
  bgfx::submit(viewId, rendering::ShaderManager::getInstance().getProgram(
                           "vs_text.bin", viewId == rendering::bga_view
                                              ? "fs_text.bin"
                                              : "fs_bgalayer.bin"));
}

long long Jukebox::getTimeMicros() { return stopwatch->elapsedMicros(); }
void Jukebox::pause() {
  SDL_Log("Pausing");
  stopwatch->pause();
}
void Jukebox::resume() { stopwatch->resume(); }
bool Jukebox::isPaused() { return !stopwatch->isRunning(); }
void Jukebox::stop() {
  isPlaying = false;
  if (playThread.joinable())
    playThread.join();
  audio.stopSounds();
  for (auto &videoPlayer : videoPlayerTable) {
    videoPlayer.second->stop();
  }
}
void Jukebox::seek(long long micro) {
  /* TODO: should also play audio/video which starts earlier than seek but
      ends later than seek */
  std::lock_guard<std::mutex> lock(seekLock);
  stopwatch->seek(micro);
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
