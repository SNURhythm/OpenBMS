#pragma once

#include <SDL2/SDL.h>
#include <bgfx/bgfx.h>
#include <vlcpp/vlc.hpp>
#include <mutex>

class VideoPlayer {
public:
  VideoPlayer();
  ~VideoPlayer();
  bool loadVideo(const std::string& videoPath);
  void update();
  void render();
  void play();
  void pause();
  void stop();
  float viewWidth = 1920.0f;
  float viewHeight = 1080.0f;
  float viewX = 0.0f;
  float viewY = 0.0f;
  float fps = 60.0f;
private:
  void* lock(void** planes);
  void unlock(void* picture, void* const* planes);
  void display(void* picture);

  void updateVideoTexture(unsigned int width, unsigned int height);
  std::mutex videoFrameMutex;
  void* videoFrameData;
  unsigned int videoFrameWidth;
  unsigned int videoFrameHeight;
  int currentFrame = 0;
  bool videoFrameUpdated;
  bgfx::TextureHandle videoTexture;
  bgfx::UniformHandle s_texColor;

  VLC::MediaPlayer* mediaPlayer = nullptr;
  unsigned int getPrecisePosition();
};

