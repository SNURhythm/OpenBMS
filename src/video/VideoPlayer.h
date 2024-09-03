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
  uint32_t setupFormat(char *chroma, unsigned *width, unsigned *height,
                       unsigned *pitches, unsigned *lines);
  void *lock(void **planes);
  void unlock(void *picture, void *const *planes);
  void display(void *picture);

  void updateVideoTexture(unsigned int width, unsigned int height);
  std::mutex videoFrameMutex;

  unsigned int videoFrameWidth;
  unsigned int videoFrameHeight;
  int currentFrame = 0;
  bool videoFrameUpdated;

  bgfx::UniformHandle s_texY = BGFX_INVALID_HANDLE;
  bgfx::UniformHandle s_texU = BGFX_INVALID_HANDLE;
  bgfx::UniformHandle s_texV = BGFX_INVALID_HANDLE;

  VLC::MediaPlayer *mediaPlayer = nullptr;
  unsigned int getPrecisePosition();
  int currentTextureIndex = 0;
  bgfx::TextureHandle videoTextureY = BGFX_INVALID_HANDLE;
  bgfx::TextureHandle videoTextureU = BGFX_INVALID_HANDLE;
  bgfx::TextureHandle videoTextureV = BGFX_INVALID_HANDLE;
  uint8_t *videoFrameDataY = nullptr;
  uint8_t *videoFrameDataU = nullptr;
  uint8_t *videoFrameDataV = nullptr;
};
