#include "VideoPlayer.h"
#include "../rendering/common.h"
#include "../rendering/ShaderManager.h"
#include "VLCInstance.h"
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
    delete mediaPlayer;
  }
  if (bgfx::isValid(videoTexture)) {
    bgfx::destroy(videoTexture);
  }
  if (videoFrameData) {
    free(videoFrameData);
  }
}

bool VideoPlayer::initialize(const std::string& videoPath) {
  VLC::Media media(*VLCInstance::getInstance().getVLCInstance(), videoPath, VLC::Media::FromPath);
  mediaPlayer = new VLC::MediaPlayer(media);
  mediaPlayer->setVideoCallbacks([this](void** planes) { return lock(planes); },
                                 [this](void* picture, void* const* planes) { unlock(picture, planes); },
                                 [this](void* picture) { display(picture); });
  media.parseWithOptions(VLC::Media::ParseFlags::Local, 10000);
  while (media.parsedStatus() != VLC::Media::ParsedStatus::Done) {
    SDL_Delay(10);
  }

  unsigned int width = media.tracks().at(0).width();
  unsigned int height = media.tracks().at(0).height();
  SDL_Log("Video dimensions: %dx%d", width, height);

  updateVideoTexture(width, height);
  mediaPlayer->setPosition(0.0f);

  bool ready = false;
  mediaPlayer->eventManager().onPlaying([&ready]() {
    ready = true;
  });

  mediaPlayer->play();
  mediaPlayer->setPause(true);
  mediaPlayer->setTime(0.0f);


  while (!ready) {
    SDL_Delay(10);
  }
  SDL_Log("Video ready");
  return true;
}

void VideoPlayer::update() {
  if (videoFrameUpdated) {
    std::lock_guard<std::mutex> lock(videoFrameMutex);
    videoFrameUpdated = false;


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

//  SDL_Log("Rendering video texture frame %d; time: %f", currentFrame, currentFrame / 30.0f);

  bgfx::VertexLayout layout;
  layout.begin()
      .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
      .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
      .end();
  bgfx::allocTransientVertexBuffer(&tvb, 4, layout);
  bgfx::allocTransientIndexBuffer(&tib, 6);
  auto* vertex = (PosTexCoord0Vertex*)tvb.data;

  // Define quad vertices
  vertex[0].x = 0.0f; vertex[0].y = viewHeight; vertex[0].z = 0.0f; vertex[0].u = 0.0f; vertex[0].v = 1.0f;
  vertex[1].x = viewWidth; vertex[1].y = viewHeight; vertex[1].z = 0.0f; vertex[1].u = 1.0f; vertex[1].v = 1.0f;
  vertex[2].x = 0.0f; vertex[2].y =  0.0f; vertex[2].z = 0.0f; vertex[2].u = 0.0f; vertex[2].v = 0.0f;
  vertex[3].x = viewWidth; vertex[3].y = 0.0f; vertex[3].z = 0.0f; vertex[3].u = 1.0f; vertex[3].v = 0.0f;

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
//  mediaPlayer->play();
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
  currentFrame++;
}

void VideoPlayer::updateVideoTexture(unsigned int width, unsigned int height) {
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
