#include "TextInputBox.h"
#include "../rendering/common.h"
#include "../rendering/ShaderManager.h"
TextInputBox::TextInputBox(const std::string &fontPath, int fontSize)
    : TextView(fontPath, fontSize) {}

TextInputBox::~TextInputBox() {}

void TextInputBox::handleEvents(SDL_Event &event) {
  if (event.type == SDL_TEXTINPUT) {

    text += event.text.text;
    SDL_Log("TextInput: %s", text.c_str());
  } else if (event.type == SDL_KEYDOWN) {
    if (event.key.keysym.sym == SDLK_BACKSPACE && text.length() > 0) {
      text.pop_back();
    }
  } else if (event.type == SDL_MOUSEBUTTONDOWN) {
    onSelected();
  }

  setText(text);
}

void TextInputBox::render() {
  TextView::render();
  if (isSelected) {
    bgfx::TransientVertexBuffer tvb;
    bgfx::TransientIndexBuffer tib;
    auto caretX = getX() + rect.w + 2;

    rendering::createRect(tvb, tib, caretX, getY(), 2, rect.h, 0xff0000ff);
    bgfx::setVertexBuffer(0, &tvb);
    bgfx::setIndexBuffer(&tib);
    bgfx::submit(
        rendering::ui_view,
        rendering::ShaderManager::getInstance().getProgram(SHADER_SIMPLE));
  }
}

void TextInputBox::onSelected() { isSelected = true; }

void TextInputBox::onUnselected() { isSelected = false; }