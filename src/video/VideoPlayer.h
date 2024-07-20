#pragma once

#include <SDL2/SDL.h>
#include <bgfx/bgfx.h>
#include <vlcpp/vlc.hpp>
#include <mutex>

class VideoPlayer {
public:
  VideoPlayer();
  ~VideoPlayer();
  bool initialize(const std::string& videoPath);
  void update();
  void render();
  void play();
  void pause();
  void stop();
  float viewWidth = 1920;
  float viewHeight = 1080;
  float viewX = 0;
  float viewY = 0;

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

  VLC::MediaPlayer* mediaPlayer;
};

