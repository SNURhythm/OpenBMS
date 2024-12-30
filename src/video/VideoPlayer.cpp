#include "VideoPlayer.h"
#include "../rendering/common.h"
#include "../rendering/ShaderManager.h"
#include <bgfx/platform.h>
#include <iostream>
#include <cstring>
#include "../rendering/common.h"

#include <thread>
#include <future>
VideoPlayer::VideoPlayer(Stopwatch *stopwatch)
    : stopwatch(stopwatch), videoFrameWidth(0), videoFrameHeight(0),
      hasVideoFrame(false), videoFrameDataY(nullptr), videoFrameDataU(nullptr),
      videoFrameDataV(nullptr) {

  s_texY = bgfx::createUniform("s_texY", bgfx::UniformType::Sampler);
  s_texU = bgfx::createUniform("s_texU", bgfx::UniformType::Sampler);
  s_texV = bgfx::createUniform("s_texV", bgfx::UniformType::Sampler);
  frameBuffer.resize(maxBufferSize, nullptr);
}

VideoPlayer::~VideoPlayer() {

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
  stopPredecoding();
  {
    std::lock_guard<std::mutex> videoLock(videoMutex);
    if (frame) {
      av_frame_free(&frame);
      frame = nullptr;
    }
    if (packet) {
      av_packet_free(&packet);
      packet = nullptr;
    }
    if (swsContext) {
      sws_freeContext(swsContext);
      swsContext = nullptr;
    }
    if (formatContext) {
      avformat_close_input(&formatContext);
      formatContext = nullptr;
    }
    if (codecContext) {
      avcodec_free_context(&codecContext);
      codecContext = nullptr;
    }
  }
}

bool VideoPlayer::loadVideo(const std::string &videoPath,
                            std::atomic<bool> &isCancelled) {
  unloadVideo();
  {
    std::lock_guard<std::mutex> videoLock(videoMutex);
    AVFormatContext *tempFormatContext = avformat_alloc_context();
    // genpts
    tempFormatContext->flags |= AVFMT_FLAG_GENPTS | AVFMT_FLAG_SORT_DTS;
    if (avformat_open_input(&tempFormatContext, videoPath.c_str(), nullptr,
                            nullptr) < 0) {
      return false;
    }
    if (avformat_find_stream_info(tempFormatContext, nullptr) < 0) {
      avformat_close_input(&tempFormatContext);
      return false;
    }

    for (unsigned i = 0; i < tempFormatContext->nb_streams; i++) {
      if (tempFormatContext->streams[i]->codecpar->codec_type ==
          AVMEDIA_TYPE_VIDEO) {
        videoStreamIndex = i;
        break;
      }
    }

    if (videoStreamIndex == -1) {
      avformat_close_input(&tempFormatContext);
      return false;
    }

    const AVCodec *codec = avcodec_find_decoder(
        tempFormatContext->streams[videoStreamIndex]->codecpar->codec_id);
    if (!codec) {
      avformat_close_input(&tempFormatContext);
      return false;
    }
    codecContext = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(
        codecContext, tempFormatContext->streams[videoStreamIndex]->codecpar);

    // Fix missing extradata (SPS/PPS)
    if (!codecContext->extradata || codecContext->extradata_size <= 0) {
      SDL_Log("Fixing missing SPS/PPS extradata");
      AVCodecParameters *codecParams =
          tempFormatContext->streams[videoStreamIndex]->codecpar;
      if (codecParams->extradata_size > 0 && codecParams->extradata) {
        codecContext->extradata = (uint8_t *)av_mallocz(
            codecParams->extradata_size + AV_INPUT_BUFFER_PADDING_SIZE);
        memcpy(codecContext->extradata, codecParams->extradata,
               codecParams->extradata_size);
        codecContext->extradata_size = codecParams->extradata_size;
      }
    }
    if (avcodec_open2(codecContext, codec, nullptr) < 0) {
      avcodec_free_context(&codecContext);
      avformat_close_input(&tempFormatContext);
      return false;
    }
    startPTS = tempFormatContext->streams[videoStreamIndex]->start_time;
    if (startPTS == AV_NOPTS_VALUE) {
      startPTS = 0; // Default to 0 if start_time is not available
    }

    frame = av_frame_alloc();
    packet = av_packet_alloc();
    swsContext = sws_getContext(codecContext->width, codecContext->height,
                                codecContext->pix_fmt, codecContext->width,
                                codecContext->height, AV_PIX_FMT_YUV420P,
                                SWS_BILINEAR, nullptr, nullptr, nullptr);

    fps = tempFormatContext->streams[videoStreamIndex]->avg_frame_rate.num /
          tempFormatContext->streams[videoStreamIndex]->avg_frame_rate.den;

    updateVideoTexture(codecContext->width, codecContext->height);

    formatContext = tempFormatContext;
    predecodingActive = true;
    predecodeThread = std::thread(&VideoPlayer::predecodeFrames, this);
    return true;
  }
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
  if (!isPlaying)
    return;

  AVFrame *currentFrame;
  double elapsedTime;
  double frameTime;
  {
    std::lock_guard<std::mutex> lock(bufferMutex);
    // check if buffer is empty
    if (bufferSize == 0) {
      // SDL_Log("Buffer is empty");
      return;
    }
    currentFrame = frameBuffer[bufferHead];
    long long now = stopwatch->elapsedMicros();
    elapsedTime = (now - startTime) / 1000000.0;
    frameTime = (currentFrame->pts - startPTS) *
                av_q2d(formatContext->streams[videoStreamIndex]->time_base);
    if (elapsedTime < frameTime) {
      return;
    }
    frameBuffer[bufferHead] = nullptr; // Clear buffer slot
    bufferHead = (bufferHead + 1) % maxBufferSize;
    --bufferSize;
  }
  freeSpace.notify_one(); // Signal that a buffer slot is free

  if (elapsedTime > frameTime + 0.1) {
    lastFramePTS = frameTime;
    SDL_Log("Skipping frame: too late for display");
    av_frame_free(&currentFrame);
    return;
  }
  uint8_t *data[3] = {videoFrameDataY, videoFrameDataU, videoFrameDataV};
  int linesize[3] = {videoFrameWidth, videoFrameWidth / 2, videoFrameWidth / 2};

  sws_scale(swsContext, currentFrame->data, currentFrame->linesize, 0,
            codecContext->height, data, linesize);

  lastFramePTS = frameTime; // Update the last displayed frame PTS

  // Upload to BGFX textures
  bgfx::updateTexture2D(
      videoTextureY, 0, 0, 0, 0, videoFrameWidth, videoFrameHeight,
      bgfx::makeRef(videoFrameDataY, videoFrameWidth * videoFrameHeight));
  bgfx::updateTexture2D(
      videoTextureU, 0, 0, 0, 0, videoFrameWidth / 2, videoFrameHeight / 2,
      bgfx::makeRef(videoFrameDataU,
                    (videoFrameWidth / 2) * (videoFrameHeight / 2)));
  bgfx::updateTexture2D(
      videoTextureV, 0, 0, 0, 0, videoFrameWidth / 2, videoFrameHeight / 2,
      bgfx::makeRef(videoFrameDataV,
                    (videoFrameWidth / 2) * (videoFrameHeight / 2)));

  hasVideoFrame = true;

  av_frame_free(&currentFrame);
}
unsigned int VideoPlayer::getPrecisePosition() {
  // calculate the frame position in microseconds
  return static_cast<unsigned int>(lastFramePTS * 1000000);
}

void VideoPlayer::render() {
  if (!hasVideoFrame)
    return;
  if (!isPlaying) {
    return;
  }

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
    startTime = stopwatch->elapsedMicros();
  } else if (isPaused) {
    // Resume playback
    long long now = stopwatch->elapsedMicros();
    double elapsedTime = (now - startTime) / 1000000.0;
    startTime =
        now - static_cast<long long>((lastFramePTS - elapsedTime) * 1000000);
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
  std::lock_guard<std::mutex> videoLock(videoMutex);
  if (!formatContext || !codecContext || videoStreamIndex < 0)
    return;

  // Convert microseconds to stream time base
  int64_t seekTarget =
      av_rescale_q(micro, AV_TIME_BASE_Q,
                   formatContext->streams[videoStreamIndex]->time_base);

  {
    // Stop playback and clear the ring buffer
    std::lock_guard<std::mutex> lock(bufferMutex);
    avcodec_flush_buffers(codecContext);

    // Free all frames in the ring buffer
    for (size_t i = 0; i < maxBufferSize; ++i) {
      if (frameBuffer[i] != nullptr) {
        av_frame_free(&frameBuffer[i]);
        frameBuffer[i] = nullptr;
      }
    }
    bufferHead = bufferTail = 0; // Reset buffer indices
    bufferSize = 0;

    // Reset freeSpace
    bufferSize = 0;
    freeSpace.notify_all();
  }

  // Perform the seek operation
  if (av_seek_frame(formatContext, videoStreamIndex, seekTarget,
                    AVSEEK_FLAG_BACKWARD) < 0) {
    SDL_Log("Failed to seek to %lld microseconds", micro);
    return;
  }

  // Reinitialize timing
  lastFramePTS = 0;
  startTime = stopwatch->elapsedMicros();

  // Notify predecoding thread to continue from the new position
  SDL_Log("Seeked to %lld microseconds", micro);
}

void VideoPlayer::predecodeFrames() {

  while (predecodingActive) {
    {
      std::unique_lock<std::mutex> lock(bufferMutex);
      freeSpace.wait(lock, [this] { return bufferSize < maxBufferSize; });
    }

    if (!predecodingActive) {
      break;
    }
    {
      std::lock_guard<std::mutex> videoLock(videoMutex);
      if (!formatContext || !codecContext || videoStreamIndex < 0) {
        SDL_Log(
            "VideoPlayer::predecodeFrames: formatContext or codecContext or "
            "videoStreamIndex is null");
        continue;
      }
      AVPacket *packet = av_packet_alloc();
      if (av_read_frame(formatContext, packet) >= 0) {
        if (packet->stream_index == videoStreamIndex) {
          avcodec_send_packet(codecContext, packet);
          AVFrame *decodedFrame = av_frame_alloc();
          if (avcodec_receive_frame(codecContext, decodedFrame) == 0) {
            // Add frame to ring buffer
            {
              std::lock_guard<std::mutex> lock(bufferMutex);
              frameBuffer[bufferTail] = decodedFrame;
              bufferTail = (bufferTail + 1) % maxBufferSize;
              ++bufferSize;
            }
          } else {
            SDL_Log("VideoPlayer::predecodeFrames failed to receive frame");
            av_frame_free(&decodedFrame);
          }
        }
      }
      av_packet_unref(packet);
      av_packet_free(&packet);
    }
  }
  SDL_Log("VideoPlayer::predecodeFrames exited");
}

void VideoPlayer::stopPredecoding() {
  predecodingActive = false;

  // Release all semaphores to unblock any waiting threads
  bufferSize = 0;
  freeSpace.notify_all(); // Release all free space

  if (predecodeThread.joinable()) {
    predecodeThread.join();
  }

  // Clear the buffer
  {
    std::lock_guard<std::mutex> lock(bufferMutex);
    for (size_t i = 0; i < maxBufferSize; ++i) {
      if (frameBuffer[i] != nullptr) {
        av_frame_free(&frameBuffer[i]);
        frameBuffer[i] = nullptr;
      }
    }
    bufferHead = bufferTail = 0; // Reset buffer indices
    bufferSize = 0;
  }
}
