//
// Created by XF on 9/2/2024.
//

#include "BMSRenderer.h"
#include "bgfx/bgfx.h"
#include "../../rendering/common.h"
#include "../../rendering/ShaderManager.h"
#include "stb_image.h"
BMSRenderer::BMSRenderer(bms_parser::Chart *chart) {
  // flatten timeline
  for (const auto &measure : chart->Measures) {
    for (const auto &timeLine : measure->TimeLines) {
      timelines.push_back(timeLine);
    }
  }
  int width, height, channels;
  unsigned char *data =
      stbi_load("assets/img/note.png", &width, &height, &channels, 4);
  if (!data) {
    SDL_Log("Failed to load note texture");
    throw std::runtime_error("Failed to load note texture");
  }
  noteHeight = height;
  noteWidth = width;
  noteTexture =
      bgfx::createTexture2D(width, height, false, 1, bgfx::TextureFormat::RGBA8,
                            0, bgfx::copy(data, width * height * 4));
}
void BMSRenderer::render(RenderContext &context, long long micro) {
  float yOrigin = 0.0f;
  drawLine(context,
           static_cast<float>(noteHeight) / static_cast<float>(noteWidth),
           yOrigin, Color(255, 255, 255, 255));
  float y = yOrigin;
  // render timeline
  for (size_t i = state.currentTimelineIndex; i < timelines.size() && y < 20.0f;
       i++) {
    auto &timeLine = timelines[i];
    if (timeLine->Timing >= micro) {

      if (i > 0) {
        auto &prevTimeLine = timelines[i - 1];
        if (prevTimeLine->Timing + prevTimeLine->GetStopDuration() > micro) {
          y += (timeLine->BeatPosition - prevTimeLine->BeatPosition) *
               prevTimeLine->Scroll * 10.0f;
        } else {
          y += (timeLine->BeatPosition - prevTimeLine->BeatPosition) *
               prevTimeLine->Scroll * (timeLine->Timing - micro) /
               (timeLine->Timing - prevTimeLine->Timing -
                prevTimeLine->GetStopDuration()) *
               10.0f;
        }
      } else {
        y += timeLine->BeatPosition * (timeLine->Timing - micro) /
             timeLine->Timing * 10.0f;
      }

      if (timeLine->IsFirstInMeasure) {
        drawLine(context, 0.05f, y, Color(255, 255, 255, 128));
      }
    } else {
      state.currentTimelineIndex = i;
    }
    //    SDL_Log("BeatPosition: %f", timeLine->BeatPosition);
    // render notes
    for (const auto &note : timeLine->Notes) {
      if (note != nullptr) {
        if (timeLine->Timing >= micro) {
          // render note
          if (state.noteObjectMap.find(note) == state.noteObjectMap.end()) {
            state.noteObjectMap[note] =
                dynamic_cast<SpriteObject *>(getInstance(ObjectType::Note));
          }
          SpriteObject *noteObject = state.noteObjectMap[note];
          noteObject->width = 1;
          noteObject->height =
              static_cast<float>(noteHeight) / static_cast<float>(noteWidth);
          auto x = note->Lane * 1.0f;
          noteObject->transform.position = {x, y, 0.0f};
          noteObject->transform.rotation =
              Quaternion::fromEuler(0.0f, 0.0f, 0.0f);
          noteObject->visible = true;
          noteObject->render(context);
        } else {
          if (state.noteObjectMap.find(note) != state.noteObjectMap.end()) {
            recycleInstance(ObjectType::Note, state.noteObjectMap[note]);
            state.noteObjectMap.erase(note);
          }
        }
      }
    }
    // render landmine notes
    for (const auto &note : timeLine->LandmineNotes) {
      if (note != nullptr) {
        // render note
      }
    }
  }
}
void BMSRenderer::drawLine(RenderContext &context, float height, float y,
                           Color color) {
  // draw judge line
  bgfx::TransientVertexBuffer tvb{};
  bgfx::TransientIndexBuffer tib{};

  // Define the vertex layout
  bgfx::VertexLayout layout = rendering::PosColorVertex::ms_decl;

  bgfx::allocTransientVertexBuffer(&tvb, 4, layout);
  bgfx::allocTransientIndexBuffer(&tib, 6);

  auto *vertices = (rendering::PosColorVertex *)tvb.data;
  auto *index = (uint16_t *)tib.data;

  // Define the corners of the rectangle in local space (2D in X-Y plane)
  uint32_t abgr = color.toABGR();
  vertices[0] = {0.0f, y, 0.0f, abgr};
  vertices[1] = {8.0f, y, 0.0f, abgr};
  vertices[2] = {8.0f, y + height, 0.0f, abgr};
  vertices[3] = {0.0f, y + height, 0.0f, abgr};

  // Set up indices for two triangles (quad)
  index[0] = 0;
  index[1] = 1;
  index[2] = 2;
  index[3] = 2;
  index[4] = 3;
  index[5] = 0;

  // Set up state (e.g., render state, texture, shaders)
  uint64_t state = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
                   BGFX_STATE_BLEND_ALPHA | BGFX_STATE_MSAA;
  bgfx::setState(state);

  // Set the vertex and index buffers
  bgfx::setVertexBuffer(0, &tvb);
  bgfx::setIndexBuffer(&tib);

  // Submit the draw call
  bgfx::submit(
      rendering::main_view,
      rendering::ShaderManager::getInstance().getProgram(SHADER_SIMPLE));
}
GameObject *BMSRenderer::getInstance(ObjectType type) {
  if (state.objectPool.find(type) == state.objectPool.end()) {
    state.objectPool[type] = std::queue<GameObject *>();
  }
  if (!state.objectPool[type].empty()) {
    GameObject *object = state.objectPool[type].front();
    state.objectPool[type].pop();
    return object;
  }
  switch (type) {
  case Note:
    return new SpriteObject(noteTexture);
  }
}
void BMSRenderer::recycleInstance(ObjectType type, GameObject *object) {
  object->visible = false;
  if (state.objectPool.find(type) == state.objectPool.end()) {
    state.objectPool[type] = std::queue<GameObject *>();
  }
  state.objectPool[type].push(object);
}
BMSRendererState::~BMSRendererState() {
  for (auto &pair : objectPool) {
    while (!pair.second.empty()) {
      delete pair.second.front();
      pair.second.pop();
    }
  }
}
