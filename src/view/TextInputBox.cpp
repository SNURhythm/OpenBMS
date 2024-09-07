#include "TextInputBox.h"
#include "../rendering/common.h"
#include "../rendering/ShaderManager.h"
#include "SDL2/SDL_events.h"
#include <cstring>
TextInputBox::TextInputBox(const std::string &fontPath, int fontSize)
    : TextView(fontPath, fontSize) {
  viewRect = {getX(), getY(), getWidth(), getHeight()};
}

TextInputBox::~TextInputBox() {}

size_t TextInputBox::getNextUnicodePos(size_t pos) {
  if (pos >= editingText.size())
    return pos;
  auto cp = editingText.data() + pos;
  while (++cp < editingText.data() + editingText.size() && (*cp & 0b10000000) &&
         !(*cp & 0b01000000)) {
  }
  return cp - editingText.data();
}
size_t TextInputBox::getPrevUnicodePos(size_t pos) {
  if (pos == 0)
    return 0;
  auto cp = editingText.data() + pos;
  while (--cp >= editingText.data() &&
         ((*cp & 0b10000000) && !(*cp & 0b01000000))) {
  }
  return cp - editingText.data();
}
void TextInputBox::handleEvents(SDL_Event &event) {
  bool shouldUpdate = false;
  bool isSubmit = false;
  switch (event.type) {
  case SDL_TEXTINPUT:
    if (!isSelected)
      return;
    composition.clear();
    shouldUpdate = true;
    // Add new text to the cursor position
    editingText.insert(cursorPos, event.text.text);
    cursorPos += strlen(event.text.text);
    break;
  case SDL_KEYDOWN:
    if (!isSelected)
      return;
    shouldUpdate = true;
    if (event.key.keysym.sym == SDLK_BACKSPACE && editingText.length() > 0) {
      if (!composition.empty()) {
        break;
      }
      // cmd + backspace - delete whole word
      if (event.key.keysym.sym == SDLK_BACKSPACE &&
          (SDL_GetModState() & KMOD_CTRL)) {
        size_t prevPos = getPrevUnicodePos(cursorPos);
        while (prevPos > 0 && !std::isspace(editingText[prevPos - 1])) {
          prevPos = getPrevUnicodePos(prevPos);
        }
        editingText.erase(prevPos, cursorPos - prevPos);
        cursorPos = prevPos;
      } else {
        size_t prevPos = getPrevUnicodePos(cursorPos);
        editingText.erase(prevPos, cursorPos - prevPos);
        cursorPos = prevPos;
      }
    } else if (event.key.keysym.sym == SDLK_DELETE &&
               editingText.length() > 0) {
      if (!composition.empty()) {
        break;
      }
      size_t nextPos = getNextUnicodePos(cursorPos);
      editingText.erase(cursorPos, nextPos - cursorPos);
    } else if (event.key.keysym.sym == SDLK_RIGHT) {
      cursorPos = getNextUnicodePos(cursorPos);

    } else if (event.key.keysym.sym == SDLK_LEFT) {
      cursorPos = getPrevUnicodePos(cursorPos);

    } // paste
    else if (event.key.keysym.sym == SDLK_v &&
             (SDL_GetModState() & KMOD_CTRL) && SDL_HasClipboardText()) {
      std::string clipboard = SDL_GetClipboardText();
      editingText.insert(cursorPos, clipboard);
      cursorPos += clipboard.size();
    } else if (event.key.keysym.sym == SDLK_RETURN) {
      isSubmit = true;
    }

    break;
  case SDL_TEXTEDITING:
    if (!isSelected)
      return;
    SDL_Log("Text editing: %s", event.edit.text);
    // Update the composition text.
    composition = event.edit.text;
    shouldUpdate = true;
    break;
  case SDL_TEXTEDITING_EXT:

    if (!isSelected)
      return;
    shouldUpdate = true;
    SDL_Log("Text editing: %s", event.editExt.text);
    // Update the composition text.
    composition = event.editExt.text;
    break;
  case SDL_MOUSEBUTTONDOWN:

    // check if the mouse is inside the text box
    if (event.button.button == SDL_BUTTON_LEFT && event.button.x >= getX() &&
        event.button.x <= getX() + getWidth() && event.button.y >= getY() &&
        event.button.y <= getY() + getHeight()) {

      cursorPos = posToCursor(event.button.x - getX(), event.button.y - getY());
      SDL_SetTextInputRect(&viewRect);
      onSelected();
      SDL_StartTextInput();
      shouldUpdate = true;
      lastRenderedCaretCursor = -1;

    } else {
      onUnselected();
      SDL_StopTextInput();
    }

    break;
  case SDL_MOUSEMOTION:
    // change mouse pointer to I-beam
    if (event.motion.x >= getX() && event.motion.x <= getX() + getWidth() &&
        event.motion.y >= getY() && event.motion.y <= getY() + getHeight()) {
      SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM));
    } else {
      SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW));
    }
    break;
  }
  if (shouldUpdate) {
    std::string composited = editingText;
    if (!composition.empty()) {
      composited.insert(cursorPos, composition);
      cursorToPos(cursorPos, editingText, compositionX, compositionY);
      compositionY += TTF_FontHeight(font);
      cursorToPos(cursorPos + composition.size(), composited, compositionWidth,
                  compositionHeight);
      compositionWidth -= compositionX;
      SDL_Log("Composition x: %d, y: %d, w: %d, h: %d", compositionX,
              compositionY, compositionWidth, compositionHeight);
    }
    setText(composited);
    for (auto &callback : onTextChangedCallbacks) {
      callback(text);
    }
    if (isSubmit) {
      for (auto &callback : onSubmitCallbacks) {
        callback(text);
      }
    }
    int cursorX, cursorY;
    cursorToPos(cursorPos, editingText, cursorX, cursorY);
    viewRect = {cursorX, cursorY, getWidth(), getHeight()};
    SDL_SetTextInputRect(&viewRect);
  }
}
void TextInputBox::onMove(int newX, int newY) {
  TextView::onMove(newX, newY);
  int cursorX, cursorY;
  cursorToPos(cursorPos, editingText, cursorX, cursorY);
  viewRect = {cursorX, cursorY, getWidth(), getHeight()};
  SDL_SetTextInputRect(&viewRect);
}
void TextInputBox::onResize(int newWidth, int newHeight) {
  TextView::onResize(newWidth, newHeight);
  int cursorX, cursorY;
  cursorToPos(cursorPos, editingText, cursorX, cursorY);
  viewRect = {cursorX, cursorY, getWidth(), getHeight()};
  SDL_SetTextInputRect(&viewRect);
}

void TextInputBox::render(RenderContext &context) {
  TextView::render(context);
  if (isSelected) {
    // Caret blink interval
    Uint32 blinkInterval = 500;
    Uint32 currentTime = SDL_GetTicks();
    if (lastRenderedCaretCursor != cursorPos) {
      lastBlink = currentTime;
      lastRenderedCaretCursor = cursorPos;
    }

    // Determine whether to show the caret
    bool showCaret =
        (currentTime - lastBlink) % (2 * blinkInterval) < blinkInterval;

    if (showCaret) {
      bgfx::TransientVertexBuffer tvb;
      bgfx::TransientIndexBuffer tib;
      int caretX, caretY;
      cursorToPos(cursorPos, editingText, caretX, caretY);
      int height = rect.h;
      if (height == 0) {
        height = TTF_FontHeight(font);
      }

      if (!composition.empty()) {
        caretX = compositionX + compositionWidth;
        caretY = compositionY;
      }

      uint32_t xcolor;
      // sdl color to abgr
      SDL_Color &c = color;
      xcolor = ((c.r << 24) | (c.g << 16) | (c.b << 8) | c.a);
      rendering::createRect(tvb, tib, caretX, getY(), 2, height, xcolor);
      bgfx::setVertexBuffer(0, &tvb);
      bgfx::setIndexBuffer(&tib);
      bgfx::setScissor(context.scissor.x, context.scissor.y,
                       context.scissor.width, context.scissor.height);
      bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
                     BGFX_STATE_BLEND_ALPHA);
      bgfx::submit(
          rendering::ui_view,
          rendering::ShaderManager::getInstance().getProgram(SHADER_SIMPLE));
    }
    // render blue underline for composition text
    if (!composition.empty()) {

      bgfx::TransientVertexBuffer tvb2;
      bgfx::TransientIndexBuffer tib2;
      rendering::createRect(tvb2, tib2, compositionX, compositionY,
                            compositionWidth, 2, 0xFFFFFFFF);
      // SDL_Log("Draw Composition x: %d, y: %d, w: %d, h: %d", compositionX,
      //         compositionY - 20, compositionWidth, 200);
      bgfx::setVertexBuffer(0, &tvb2);
      bgfx::setIndexBuffer(&tib2);
      bgfx::setScissor(context.scissor.x, context.scissor.y,
                       context.scissor.width, context.scissor.height);
      bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
                     BGFX_STATE_BLEND_ALPHA);
      bgfx::submit(
          rendering::ui_view,
          rendering::ShaderManager::getInstance().getProgram(SHADER_SIMPLE));
    }
  }
}

void TextInputBox::cursorToPos(size_t cursorPos, const std::string &text,
                               int &x, int &y) {
  // TODO: this will not work for multi-line text
  std::string utf8 = text;
  if (cursorPos > utf8.size())
    cursorPos = utf8.size();
  if (cursorPos < 0)
    cursorPos = 0;
  utf8.resize(cursorPos);

  TTF_SizeUTF8(font, utf8.c_str(), &x, &y);
  x += getX();
  y += getY() - TTF_FontHeight(font);
}

size_t TextInputBox::posToCursor(int x, int y) {
  // TODO: this will not work for multi-line text
  size_t cursorPos = 0;
  int w = 0, h = 0;
  int dw = 0;
  int dh = 0;
  size_t glyphs = 0;
  for (cursorPos = 0; cursorPos < editingText.size();) {

    int prevW = w;
    int prevH = h;
    TTF_SizeUTF8(font, editingText.substr(0, cursorPos).c_str(), &w, &h);
    if (prevW != 0)
      dw = w - prevW;
    else
      dw = w;
    if (prevH != 0)
      dh = h - prevH;
    else
      dh = h;
    if (glyphs == 1 && dw / 2 > x)
      return 0;
    if (w + dw / 2 > x) {

      break;
    }
    glyphs++;
    cursorPos = getNextUnicodePos(cursorPos);
  }
  return cursorPos;
}

void TextInputBox::onSelected() { isSelected = true; }

void TextInputBox::onUnselected() { isSelected = false; }

size_t
TextInputBox::onTextChanged(std::function<void(const std::string &)> callback) {
  onTextChangedCallbacks.push_back(callback);
  return onTextChangedCallbacks.size() - 1;
}

void TextInputBox::removeOnTextChanged(
    std::function<void(const std::string &)> callback) {
  for (size_t i = 0; i < onTextChangedCallbacks.size(); i++) {
    if (onTextChangedCallbacks[i].target<void(const std::string &)>() ==
        callback.target<void(const std::string &)>()) {
      onTextChangedCallbacks.erase(onTextChangedCallbacks.begin() + i);
    }
  }
}

size_t
TextInputBox::onSubmit(std::function<void(const std::string &)> callback) {
  onSubmitCallbacks.push_back(callback);
  return onSubmitCallbacks.size() - 1;
}

void TextInputBox::removeOnSubmit(
    std::function<void(const std::string &)> callback) {
  for (size_t i = 0; i < onSubmitCallbacks.size(); i++) {
    if (onSubmitCallbacks[i].target<void(const std::string &)>() ==
        callback.target<void(const std::string &)>()) {
      onSubmitCallbacks.erase(onSubmitCallbacks.begin() + i);
    }
  }
}