#include "VideoPlayer.h"
#include "../rendering/common.h"
#include "../rendering/ShaderManager.h"
#include "VLCInstance.h"
#include <bgfx/platform.h>
#include <iostream>
#include <cstring>
#include "../rendering/common.h"

#include <thread>
#include <future>
VideoPlayer::VideoPlayer()
    : videoFrameWidth(0), videoFrameHeight(0), hasVideoFrame(false),
      videoFrameDataY(nullptr), videoFrameDataU(nullptr),
      videoFrameDataV(nullptr) {

  s_texY = bgfx::createUniform("s_texY", bgfx::UniformType::Sampler);
  s_texU = bgfx::createUniform("s_texU", bgfx::UniformType::Sampler);
  s_texV = bgfx::createUniform("s_texV", bgfx::UniformType::Sampler);
}

VideoPlayer::~VideoPlayer() {
  if (mediaPlayer) {
    mediaPlayer->stopAsync();
    delete mediaPlayer;
  }

  bgfx::destroy(s_texY);
  bgfx::destroy(s_texU);
  bgfx::destroy(s_texV);
  unloadVideo();
  if (videoFrameDataY != nullptr) {
    free(videoFrameDataY);
    videoFrameDataY = nullptr;
  }
  if (videoFrameDataU != nullptr) {
    free(videoFrameDataU);
    videoFrameDataU = nullptr;
  }
  if (videoFrameDataV != nullptr) {
    free(videoFrameDataV);
    videoFrameDataV = nullptr;
  }
}

void VideoPlayer::unloadVideo() {
  std::lock_guard<std::mutex> lock(videoMutex);
  hasVideoFrame = false;
  stop();
  if (frame != nullptr) {
    av_frame_free(&frame);
    frame = nullptr;
  }
  if (packet != nullptr) {
    av_packet_free(&packet);
    packet = nullptr;
  }
  if (swsContext != nullptr) {
    sws_freeContext(swsContext);
    swsContext = nullptr;
  }
  if (formatContext != nullptr) {
    avformat_close_input(&formatContext);
    formatContext = nullptr;
  }
  if (codecContext != nullptr) {
    avcodec_free_context(&codecContext);
    codecContext = nullptr;
  }
}

bool VideoPlayer::loadVideo(const std::string &videoPath,
                            std::atomic<bool> &isCancelled) {
  // unload video
  unloadVideo();
  // Open video file
  AVFormatContext *formatContext = avformat_alloc_context();
  // genpts
  formatContext->flags |= AVFMT_FLAG_GENPTS;

  // faststart
  AVDictionary *opts2 = NULL;
  av_dict_set(&opts2, "movflags", "+faststart", 0);
  if (avformat_open_input(&formatContext, videoPath.c_str(), nullptr, &opts2) <
      0) {
    SDL_Log("Failed to open video file");
    return false;
  }

  // Find stream information
  if (avformat_find_stream_info(formatContext, nullptr) < 0) {
    SDL_Log("Failed to find stream info");
    avformat_close_input(&formatContext);
    return false;
  }

  // Find the video stream
  AVCodecParameters *codecParams = nullptr;
  videoStreamIndex = -1;
  for (unsigned i = 0; i < formatContext->nb_streams; i++) {
    if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      videoStreamIndex = i;
      codecParams = formatContext->streams[i]->codecpar;
      break;
    }
  }
  if (videoStreamIndex == -1) {
    SDL_Log("No video stream found");
    avformat_close_input(&formatContext);
    return false;
  }

  // Find decoder for the video stream
  const AVCodec *codec = avcodec_find_decoder(codecParams->codec_id);
  if (!codec) {
    SDL_Log("Failed to find codec");
    avformat_close_input(&formatContext);
    return false;
  }

  AVCodecContext *codecContext = avcodec_alloc_context3(codec);
  avcodec_parameters_to_context(codecContext, codecParams);
  if (avcodec_open2(codecContext, codec, nullptr) < 0) {
    SDL_Log("Failed to open codec");
    avcodec_free_context(&codecContext);
    avformat_close_input(&formatContext);
    return false;
  }

  // Initialize frame, packet, and scaling context
  AVFrame *frame = av_frame_alloc();
  AVPacket *packet = av_packet_alloc();
  SwsContext *swsContext = sws_getContext(
      codecContext->width, codecContext->height, codecContext->pix_fmt,
      codecContext->width, codecContext->height, AV_PIX_FMT_YUV420P,
      SWS_BILINEAR, nullptr, nullptr, nullptr);

  // Update textures
  updateVideoTexture(codecContext->width, codecContext->height);

  // Store contexts for later use
  this->formatContext = formatContext;
  this->codecContext = codecContext;
  this->frame = frame;
  this->packet = packet;
  this->swsContext = swsContext;

  return true;
}

uint32_t VideoPlayer::setupFormat(char *chroma, unsigned *width,
                                  unsigned *height, unsigned *pitches,
                                  unsigned *lines) {
  if (chroma == nullptr || width == nullptr || height == nullptr ||
      pitches == nullptr || lines == nullptr)
    return 0;
  chroma[0] = 'I';
  chroma[1] = '4';
  chroma[2] = '2';
  chroma[3] = '0';
  chroma[4] = '\0';
  *width = videoFrameWidth;
  *height = videoFrameHeight;
  pitches[0] = videoFrameWidth;
  pitches[1] = videoFrameWidth / 2;
  pitches[2] = videoFrameWidth / 2;
  lines[0] = videoFrameHeight;
  lines[1] = videoFrameHeight / 2;
  lines[2] = videoFrameHeight / 2;
  return 1;
}

void VideoPlayer::update() {
  std::lock_guard<std::mutex> lock(videoMutex);
  if (!isPlaying || isPaused) {
    return; // Do nothing if playback is stopped or paused
  }

  if (stopRequested) {
    // Cleanup resources for stopping
    hasVideoFrame = false;
    avcodec_flush_buffers(codecContext);
    av_seek_frame(formatContext, videoStreamIndex, 0, AVSEEK_FLAG_BACKWARD);
    stopRequested = false;
    return;
  }

  // Handle seeking
  if (seekPosition >= 0) {
    avcodec_flush_buffers(codecContext);
    int ret = av_seek_frame(
        formatContext, videoStreamIndex,
        seekPosition /
            (1000 *
             av_q2d(formatContext->streams[videoStreamIndex]->time_base)),
        AVSEEK_FLAG_BACKWARD);
    if (ret < 0) {
      SDL_Log("Seek failed");
    } else {
      SDL_Log("Seek successful");
      startTime = std::chrono::high_resolution_clock::now();
    }
    seekPosition = -1;
  }

  // Decode and render frames
  while (true) {
    if (av_read_frame(formatContext, packet) >= 0) {
      if (packet->stream_index == videoStreamIndex) {
        avcodec_send_packet(codecContext, packet);
        if (avcodec_receive_frame(codecContext, frame) == 0) {
          if (frame->pts == AV_NOPTS_VALUE) {
            SDL_Log("Corrupted PTS, recalculating...");
            AVRational timeBase =
                formatContext->streams[videoStreamIndex]->time_base;
            frame->pts = lastFramePTS + (1 / av_q2d(timeBase));
          }
          uint8_t *data[3] = {videoFrameDataY, videoFrameDataU,
                              videoFrameDataV};
          int linesize[3] = {videoFrameWidth, videoFrameWidth / 2,
                             videoFrameWidth / 2};

          // Convert frame to YUV420P
          sws_scale(swsContext, frame->data, frame->linesize, 0,
                    codecContext->height, data, linesize);

          // Synchronize frame timing based on PTS
          double frameTime =
              frame->pts *
              av_q2d(formatContext->streams[videoStreamIndex]->time_base);

          auto now = std::chrono::high_resolution_clock::now();
          double elapsedTime =
              std::chrono::duration_cast<std::chrono::milliseconds>(now -
                                                                    startTime)
                  .count() /
              1000.0;

          if (elapsedTime < frameTime) {
            std::this_thread::sleep_for(std::chrono::milliseconds(
                static_cast<int>((frameTime - elapsedTime) * 1000)));
          } else if (elapsedTime > frameTime + 0.1) {
            SDL_Log("Skipping frame: too late for display");
            continue; // Skip the frame if it's too late
          }

          lastFramePTS = frameTime; // Update the last displayed frame PTS

          // Upload to BGFX textures
          bgfx::updateTexture2D(
              videoTextureY, 0, 0, 0, 0, videoFrameWidth, videoFrameHeight,
              bgfx::makeRef(videoFrameDataY,
                            videoFrameWidth * videoFrameHeight));
          bgfx::updateTexture2D(
              videoTextureU, 0, 0, 0, 0, videoFrameWidth / 2,
              videoFrameHeight / 2,
              bgfx::makeRef(videoFrameDataU,
                            (videoFrameWidth / 2) * (videoFrameHeight / 2)));
          bgfx::updateTexture2D(
              videoTextureV, 0, 0, 0, 0, videoFrameWidth / 2,
              videoFrameHeight / 2,
              bgfx::makeRef(videoFrameDataV,
                            (videoFrameWidth / 2) * (videoFrameHeight / 2)));

          hasVideoFrame = true;
        }
      }
      av_packet_unref(packet);
    }
    break;
  }
}

unsigned int VideoPlayer::getPrecisePosition() {
  return currentFrame * 1000 / fps;
}

void VideoPlayer::render() {
  if (!hasVideoFrame)
    return;
  // Submit a quad with the video texture
  bgfx::TransientVertexBuffer tvb{};
  bgfx::TransientIndexBuffer tib{};

  //  SDL_Log("Rendering video texture frame %d; time: %f", currentFrame,
  //  currentFrame / 30.0f);

  bgfx::VertexLayout &layout = rendering::PosTexCoord0Vertex::ms_decl;
  bgfx::allocTransientVertexBuffer(&tvb, 4, layout);
  bgfx::allocTransientIndexBuffer(&tib, 6);
  auto *vertex = (rendering::PosTexCoord0Vertex *)tvb.data;

  // Define quad vertices
  vertex[0].x = 0.0f;
  vertex[0].y = viewHeight;
  vertex[0].z = 0.0f;
  vertex[0].u = 0.0f;
  vertex[0].v = 1.0f;

  vertex[1].x = viewWidth;
  vertex[1].y = viewHeight;
  vertex[1].z = 0.0f;
  vertex[1].u = 1.0f;
  vertex[1].v = 1.0f;

  vertex[2].x = 0.0f;
  vertex[2].y = 0.0f;
  vertex[2].z = 0.0f;
  vertex[2].u = 0.0f;
  vertex[2].v = 0.0f;

  vertex[3].x = viewWidth;
  vertex[3].y = 0.0f;
  vertex[3].z = 0.0f;
  vertex[3].u = 1.0f;
  vertex[3].v = 0.0f;

  // Define quad indices
  auto *indices = (uint16_t *)tib.data;
  indices[0] = 0;
  indices[1] = 1;
  indices[2] = 2;
  indices[3] = 1;
  indices[4] = 3;
  indices[5] = 2;
  bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
                 BGFX_STATE_BLEND_ALPHA);
  bgfx::setVertexBuffer(0, &tvb);
  bgfx::setIndexBuffer(&tib);
  bgfx::setTexture(0, s_texY, videoTextureY);
  bgfx::setTexture(1, s_texU, videoTextureU);
  bgfx::setTexture(2, s_texV, videoTextureV);

  bgfx::submit(viewId, rendering::ShaderManager::getInstance().getProgram(
                           SHADER_YUVRGB));
}

void VideoPlayer::play() {
  if (!isPlaying) {
    // Start playback
    startTime = std::chrono::high_resolution_clock::now();
  } else if (isPaused) {
    // Resume playback
    auto now = std::chrono::high_resolution_clock::now();
    double elapsedTime =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime)
            .count() /
        1000.0;
    startTime = now - std::chrono::milliseconds(static_cast<int>(
                          (lastFramePTS - elapsedTime) * 1000));
  }
  isPlaying = true;
  isPaused = false;
  stopRequested = false;
}

void VideoPlayer::pause() { isPaused = true; }

void VideoPlayer::stop() {
  isPlaying = false;
  stopRequested = true;
  seekPosition = -1; // Reset seek position
}

void VideoPlayer::updateVideoTexture(unsigned int width, unsigned int height) {
  std::lock_guard<std::mutex> lock(videoFrameMutex);
  if (width != videoFrameWidth || height != videoFrameHeight) {
    if (bgfx::isValid(videoTextureY))
      bgfx::destroy(videoTextureY);
    if (bgfx::isValid(videoTextureU))
      bgfx::destroy(videoTextureU);
    if (bgfx::isValid(videoTextureV))
      bgfx::destroy(videoTextureV);
    if (videoFrameDataY != nullptr) {
      free(videoFrameDataY);
      videoFrameDataY = nullptr;
    }
    if (videoFrameDataU != nullptr) {
      free(videoFrameDataU);
      videoFrameDataU = nullptr;
    }
    if (videoFrameDataV != nullptr) {
      free(videoFrameDataV);
      videoFrameDataV = nullptr;
    }

    videoFrameWidth = width;
    videoFrameHeight = height;

    // Create textures for Y, U, and V planes
    videoTextureY = bgfx::createTexture2D(
        uint16_t(videoFrameWidth), uint16_t(videoFrameHeight), false, 1,
        bgfx::TextureFormat::R8, BGFX_TEXTURE_NONE | BGFX_SAMPLER_NONE);

    videoTextureU = bgfx::createTexture2D(
        uint16_t(videoFrameWidth / 2), uint16_t(videoFrameHeight / 2), false, 1,
        bgfx::TextureFormat::R8, BGFX_TEXTURE_NONE | BGFX_SAMPLER_NONE);

    videoTextureV = bgfx::createTexture2D(
        uint16_t(videoFrameWidth / 2), uint16_t(videoFrameHeight / 2), false, 1,
        bgfx::TextureFormat::R8, BGFX_TEXTURE_NONE | BGFX_SAMPLER_NONE);

    // Allocate memory for YUV data

    videoFrameDataY =
        new uint8_t[videoFrameWidth * videoFrameHeight]; // Y plane
    videoFrameDataU =
        new uint8_t[videoFrameWidth * videoFrameHeight / 4]; // U plane
    videoFrameDataV =
        new uint8_t[videoFrameWidth * videoFrameHeight / 4]; // V plane

    // Check if memory allocation was successful
    if (!videoFrameDataY || !videoFrameDataU || !videoFrameDataV) {
      SDL_Log("Failed to allocate memory for YUV data");
      return;
    }
  }
}

void VideoPlayer::seek(int64_t micro) {
  if (isPlaying || isPaused) {
    seekPosition = micro;
  }
}
