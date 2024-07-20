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
  void updateVideoTexture(int width, int height);
private:
  void* lock(void** planes);
  void unlock(void* picture, void* const* planes);
  void display(void* picture);


  std::mutex videoFrameMutex;
  void* videoFrameData;
  int videoFrameWidth;
  int videoFrameHeight;
  int currentFrame = 0;
  bool videoFrameUpdated;
  bgfx::TextureHandle videoTexture;
  bgfx::UniformHandle s_texColor;

  VLC::MediaPlayer* mediaPlayer;
};

