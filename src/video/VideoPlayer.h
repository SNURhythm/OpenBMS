#pragma once

#include <SDL2/SDL.h>
#include <bgfx/bgfx.h>
#include <vlcpp/vlc.hpp>
#include <mutex>

class VideoPlayer {
public:
  VideoPlayer();
  ~VideoPlayer();
  bool loadVideo(const std::string &videoPath);
  void update();
  void render();
  void play();
  void pause();
  void stop();
  void seek(long long micro);
  float viewWidth = 1920.0f;
  float viewHeight = 1080.0f;
  float viewX = 0.0f;
  float viewY = 0.0f;
  float fps = 60.0f;

private:
  void *lock(void **planes);
  void unlock(void *picture, void *const *planes);
  void display(void *picture);

  void updateVideoTexture(unsigned int width, unsigned int height);
  std::mutex videoFrameMutex;
  void *videoFrameData;
  unsigned int videoFrameWidth;
  unsigned int videoFrameHeight;
  int currentFrame = 0;
  bool videoFrameUpdated;
  bgfx::TextureHandle videoTextures[2] = {BGFX_INVALID_HANDLE,
                                          BGFX_INVALID_HANDLE};
  bgfx::UniformHandle s_texColor = BGFX_INVALID_HANDLE;
  bgfx::UniformHandle s_texY = BGFX_INVALID_HANDLE;
  bgfx::UniformHandle s_texU = BGFX_INVALID_HANDLE;
  bgfx::UniformHandle s_texV = BGFX_INVALID_HANDLE;

  VLC::MediaPlayer *mediaPlayer = nullptr;
  unsigned int getPrecisePosition();
  int currentTextureIndex = 0;
  bgfx::TextureHandle videoTextureY = BGFX_INVALID_HANDLE;
  bgfx::TextureHandle videoTextureU = BGFX_INVALID_HANDLE;
  bgfx::TextureHandle videoTextureV = BGFX_INVALID_HANDLE;
  void *videoFrameDataY;
  void *videoFrameDataU;
  void *videoFrameDataV;
};
