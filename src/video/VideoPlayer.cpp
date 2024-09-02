#include "VideoPlayer.h"
#include "../rendering/common.h"
#include "../rendering/ShaderManager.h"
#include "VLCInstance.h"
#include <bgfx/platform.h>
#include <iostream>
#include <cstring>
#include "../rendering/common.h"
VideoPlayer::VideoPlayer()
    : videoFrameWidth(0), videoFrameHeight(0), videoFrameUpdated(false),
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

bool VideoPlayer::loadVideo(const std::string &videoPath) {
  SDL_Log("Loading video: %s", videoPath.c_str());
  VLC::Media media(videoPath, VLC::Media::FromPath);
  if (mediaPlayer) {
    mediaPlayer->stopAsync();
    delete mediaPlayer;
  }

  currentFrame = 0;
  auto &instance = *VLCInstance::getInstance().getVLCInstance();
  mediaPlayer = new VLC::MediaPlayer(instance, media);

  media.parseRequest(instance,
                     VLC::Media::ParseFlags::Local |
                         VLC::Media::ParseFlags::FetchLocal,
                     10000);
  while (media.parsedStatus(instance) != VLC::Media::ParsedStatus::Done) {
    if (media.parsedStatus(instance) == VLC::Media::ParsedStatus::Failed) {
      SDL_Log("Failed to parse video");
      return false;
    }
    SDL_Delay(10);
  }
  bool hasVideo = false;
  for (auto &track : media.tracks(VLC::MediaTrack::Type::Video)) {
    if (track.type() == VLC::MediaTrack::Type::Video) {
      unsigned int width = track.width();
      unsigned int height = track.height();
      fps = track.fpsNum() / static_cast<float>(track.fpsDen());
      SDL_Log("Video FPS: %f; fpsNum: %d, fpsDen: %d", fps, track.fpsNum(),
              track.fpsDen());
      if (width == 0 || height == 0)
        continue;
      SDL_Log("Video dimensions: %dx%d", width, height);
      updateVideoTexture(width, height);
      hasVideo = true;
      break;
    }
  }
  if (!hasVideo) {
    updateVideoTexture(1920, 1080);
  }
  mediaPlayer->setVideoFormat("I420", videoFrameWidth, videoFrameHeight,
                              videoFrameWidth);
  mediaPlayer->setVideoCallbacks(
      [this](void **planes) { return lock(planes); },
      [this](void *picture, void *const *planes) { unlock(picture, planes); },
      [this](void *picture) { display(picture); });

  mediaPlayer->setPosition(0.0f, true);

  mediaPlayer->play();
  mediaPlayer->setPause(true);
  mediaPlayer->setTime(0.0f, true);
  //

  SDL_Log("Video ready");
  return true;
}

void VideoPlayer::update() {
  if (videoFrameUpdated) {
    std::lock_guard<std::mutex> lock(videoFrameMutex);
    videoFrameUpdated = false;

    const bgfx::Memory *memY =
        bgfx::makeRef(videoFrameDataY, videoFrameWidth * videoFrameHeight);
    const bgfx::Memory *memU =
        bgfx::makeRef(videoFrameDataU, videoFrameWidth * videoFrameHeight / 4);
    const bgfx::Memory *memV =
        bgfx::makeRef(videoFrameDataV, videoFrameWidth * videoFrameHeight / 4);

    bgfx::updateTexture2D(videoTextureY, 0, 0, 0, 0, videoFrameWidth,
                          videoFrameHeight, memY);
    bgfx::updateTexture2D(videoTextureU, 0, 0, 0, 0, videoFrameWidth / 2,
                          videoFrameHeight / 2, memU);
    bgfx::updateTexture2D(videoTextureV, 0, 0, 0, 0, videoFrameWidth / 2,
                          videoFrameHeight / 2, memV);
  }
}

unsigned int VideoPlayer::getPrecisePosition() {
  return currentFrame * 1000 / fps;
}

void VideoPlayer::render() {
  //  return;
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

  bgfx::submit(
      rendering::bga_view,
      rendering::ShaderManager::getInstance().getProgram(SHADER_YUVRGB));
}

void VideoPlayer::play() { mediaPlayer->play(); }

void VideoPlayer::pause() { mediaPlayer->pause(); }

void VideoPlayer::stop() { mediaPlayer->stopAsync(); }

void *VideoPlayer::lock(void **planes) {
  std::lock_guard<std::mutex> lock(videoFrameMutex);
  planes[0] = videoFrameDataY; // Y
  planes[1] = videoFrameDataU; // U
  planes[2] = videoFrameDataV; // V
  return nullptr;
}

void VideoPlayer::unlock(void *picture, void *const *planes) {
  std::lock_guard<std::mutex> lock(videoFrameMutex);
  videoFrameUpdated = true;
}

void VideoPlayer::display(void *picture) {
  // No additional processing needed here
  currentFrame++;
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

    videoFrameDataY = malloc(videoFrameWidth * videoFrameHeight); // Y plane
    videoFrameDataU =
        malloc((videoFrameWidth / 2) * (videoFrameHeight / 2)); // U plane
    videoFrameDataV =
        malloc((videoFrameWidth / 2) * (videoFrameHeight / 2)); // V plane

    // Check if memory allocation was successful
    if (!videoFrameDataY || !videoFrameDataU || !videoFrameDataV) {
      SDL_Log("Failed to allocate memory for YUV data");
      return;
    }
  }
}

void VideoPlayer::seek(long long int micro) {
  mediaPlayer->setTime(micro / 1000.0f, true);
}
