#include "VideoPlayer.h"
#include "../rendering/common.h"
#include "../rendering/ShaderManager.h"
#include <bgfx/platform.h>
#include <iostream>
#include <cstring>

VideoPlayer::VideoPlayer()
    : videoFrameData(nullptr), videoFrameWidth(0), videoFrameHeight(0), videoFrameUpdated(false), videoTexture(BGFX_INVALID_HANDLE) {
  s_texColor = bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);
}

VideoPlayer::~VideoPlayer() {
  if (mediaPlayer) {
    mediaPlayer->stop();
  }
  if (bgfx::isValid(videoTexture)) {
    bgfx::destroy(videoTexture);
  }
  if (videoFrameData) {
    free(videoFrameData);
  }
}

bool VideoPlayer::initialize(const VLC::Instance& instance, const std::string& videoPath) {
  VLC::Media media(instance, videoPath, VLC::Media::FromPath);
  mediaPlayer = std::make_unique<VLC::MediaPlayer>(media);
  mediaPlayer->setVideoCallbacks([this](void** planes) { return lock(planes); },
                                 [this](void* picture, void* const* planes) { unlock(picture, planes); },
                                 [this](void* picture) { display(picture); });

  return true;
}

void VideoPlayer::update() {
  if (videoFrameUpdated) {
    std::lock_guard<std::mutex> lock(videoFrameMutex);
    videoFrameUpdated = false;

    unsigned int width, height;
    mediaPlayer->size(0, &width, &height);
    SDL_Log("VideoPlayer::update: %d x %d", width, height);

    if (bgfx::isValid(videoTexture)) {
      const bgfx::Memory* mem = bgfx::makeRef(videoFrameData, videoFrameWidth * videoFrameHeight * 4);
      bgfx::updateTexture2D(videoTexture, 0, 0, 0, 0, videoFrameWidth, videoFrameHeight, mem);
    }
  }
}

void VideoPlayer::render() {
  // Submit a quad with the video texture
  bgfx::TransientVertexBuffer tvb{};
  bgfx::TransientIndexBuffer tib{};

  struct PosTexCoord0Vertex {
    float x, y, z;
    float u, v;
  };

  bgfx::VertexLayout layout;
  layout.begin()
      .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
      .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
      .end();
  bgfx::allocTransientVertexBuffer(&tvb, 4, layout);
  bgfx::allocTransientIndexBuffer(&tib, 6);
  auto* vertex = (PosTexCoord0Vertex*)tvb.data;

  // Define quad vertices
  vertex[0].x = -1.0f; vertex[0].y = -1.0f; vertex[0].z = 0.0f; vertex[0].u = 0.0f; vertex[0].v = 1.0f;
  vertex[1].x =  1.0f; vertex[1].y = -1.0f; vertex[1].z = 0.0f; vertex[1].u = 1.0f; vertex[1].v = 1.0f;
  vertex[2].x = -1.0f; vertex[2].y =  1.0f; vertex[2].z = 0.0f; vertex[2].u = 0.0f; vertex[2].v = 0.0f;
  vertex[3].x =  1.0f; vertex[3].y =  1.0f; vertex[3].z = 0.0f; vertex[3].u = 1.0f; vertex[3].v = 0.0f;

  // Define quad indices
  auto* indices = (uint16_t*)tib.data;
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
  bgfx::setTexture(0, s_texColor, videoTexture);

  bgfx::submit(
      rendering::bga_view,
      rendering::ShaderManager::getInstance().getProgram(SHADER_TEXT));
}

void VideoPlayer::play() {
  mediaPlayer->play();
}

void VideoPlayer::pause() {
  mediaPlayer->pause();
}

void VideoPlayer::stop() {
  mediaPlayer->stop();
}

void* VideoPlayer::lock(void** planes) {
  std::lock_guard<std::mutex> lock(videoFrameMutex);
  *planes = videoFrameData;
  return nullptr;
}

void VideoPlayer::unlock(void* picture, void* const* planes) {
  std::lock_guard<std::mutex> lock(videoFrameMutex);
  videoFrameUpdated = true;
}

void VideoPlayer::display(void* picture) {
  // No additional processing needed here
}

void VideoPlayer::updateVideoTexture(int width, int height) {
  if (width != videoFrameWidth || height != videoFrameHeight) {
    if (bgfx::isValid(videoTexture)) {
      bgfx::destroy(videoTexture);
    }
    videoFrameWidth = width;
    videoFrameHeight = height;

    videoTexture = bgfx::createTexture2D(
        uint16_t(videoFrameWidth),
        uint16_t(videoFrameHeight),
        false,
        1,
        bgfx::TextureFormat::BGRA8,
        BGFX_TEXTURE_NONE | BGFX_SAMPLER_NONE
    );

    if (videoFrameData) {
      free(videoFrameData);
    }

    mediaPlayer->setVideoFormat("RV32", videoFrameWidth, videoFrameHeight, videoFrameWidth * 4);

    videoFrameData = malloc(videoFrameWidth * videoFrameHeight * 4); // Assuming 32-bit RGBA
  }
}
