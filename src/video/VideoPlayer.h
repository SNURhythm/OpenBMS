#pragma once

#include <SDL2/SDL.h>
#include <bgfx/bgfx.h>
#include <vlcpp/vlc.hpp>
#include <mutex>
#include <future>
#include "../rendering/common.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

class VideoPlayer {
public:
  VideoPlayer();
  ~VideoPlayer();

  bool loadVideo(const std::string &videoPath, std::atomic<bool> &isCancelled);
  void update();
  void render();
  void play();
  void pause();
  void stop();
  void seek(int64_t micro);
  float viewWidth = 1920.0f;
  float viewHeight = 1080.0f;
  int viewId = rendering::bga_view;
  float viewX = 0.0f;
  float viewY = 0.0f;
  float fps = 60.0f;

private:
  std::atomic<bool> isPlaying;
  std::atomic<bool> isPaused;
  std::atomic<bool> stopRequested;
  std::atomic<bool> predecodingActive;
  std::atomic<int64_t> seekPosition;
  std::thread predecodeThread;

  AVFormatContext *formatContext = nullptr;
  AVCodecContext *codecContext = nullptr;
  AVFrame *frame = nullptr;
  AVPacket *packet = nullptr;
  SwsContext *swsContext = nullptr;
  int videoStreamIndex = -1;
  std::mutex videoMutex;

  std::queue<AVFrame *> frameBuffer;
  std::mutex frameBufferMutex;
  std::condition_variable frameBufferCV;
  size_t maxBufferSize = 30;

  std::chrono::high_resolution_clock::time_point
      startTime;             // Start time for playback
  double lastFramePTS = 0.0; // Last decoded frame's PTS for synchronization

  std::queue<AVFrame *> frameQueue; // Queue for pre-decoded frames
  std::mutex bufferMutex;           // Protect access to the frame queue
  std::condition_variable bufferCV; // Condition variable for buffer signaling

  const size_t maxBufferFrames = 120; // Maximum number of frames in the buffer
  bool isBuffering = false;           // Indicates if buffering is in progress

  uint32_t setupFormat(char *chroma, unsigned *width, unsigned *height,
                       unsigned *pitches, unsigned *lines);
  void unloadVideo();
  void predecodeFrames();
  void stopPredecoding();

  void updateVideoTexture(unsigned int width, unsigned int height);
  std::mutex videoFrameMutex;

  int videoFrameWidth;
  int videoFrameHeight;
  bool hasVideoFrame;

  bgfx::UniformHandle s_texY = BGFX_INVALID_HANDLE;
  bgfx::UniformHandle s_texU = BGFX_INVALID_HANDLE;
  bgfx::UniformHandle s_texV = BGFX_INVALID_HANDLE;

  VLC::MediaPlayer *mediaPlayer = nullptr;
  VLC::Media media;
  int64_t startPTS = 0;
  unsigned int getPrecisePosition();
  int currentTextureIndex = 0;
  bgfx::TextureHandle videoTextureY = BGFX_INVALID_HANDLE;
  bgfx::TextureHandle videoTextureU = BGFX_INVALID_HANDLE;
  bgfx::TextureHandle videoTextureV = BGFX_INVALID_HANDLE;
  uint8_t *videoFrameDataY = nullptr;
  uint8_t *videoFrameDataU = nullptr;
  uint8_t *videoFrameDataV = nullptr;
};
