#include "bgfx_helper.h"
#include "rendering/ShaderManager.h"
#include "./audio/decoder.h"
#include "bx/math.h"
#include <cstdio>

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_video.h>
#include "main.h"
#include "scene/MainMenuScene.h"
#include "scene/SceneManager.h"
#include <cstdlib>
#include <iostream>
#include <string>
#include <bgfx/bgfx.h>
#include <bgfx/embedded_shader.h>
#include <bgfx/platform.h>
#include <bx/platform.h>
#include "rendering/common.h"
#include "context.h"
#include "audio/AudioWrapper.h"
#include "view/TextView.h"
#ifdef _WIN32
#include <windows.h>

#elif __APPLE__

#include "TargetConditionals.h"
#if TARGET_OS_IPHONE && TARGET_IPHONE_SIMULATOR
// define something for simulator
#elif TARGET_OS_IPHONE
#include "iOSNatives.hpp"
// define something for iphone
#include <dirent.h>
#include <sys/stat.h>
#else
// define something for OSX
#include "MacNatives.h"
#include <dirent.h>
#include <sys/stat.h>
#endif
#elif __linux
// linux
#include <dirent.h>
#include <sys/stat.h>
#elif __unix // all unices not caught above
// Unix
#elif __posix
// POSIX
#endif
#include <sol/sol.hpp>
#include "rendering/Camera.h"

bgfx::VertexLayout rendering::PosColorVertex::ms_decl;
bgfx::VertexLayout rendering::PosTexVertex::ms_decl;
bgfx::VertexLayout rendering::PosTexCoord0Vertex::ms_decl;

static bgfx::VertexLayout s_PosTexLayout;

static bgfx::ProgramHandle s_ProgBlurH = BGFX_INVALID_HANDLE;
static bgfx::ProgramHandle s_ProgBlurV = BGFX_INVALID_HANDLE;
static bgfx::ProgramHandle s_ProgRect = BGFX_INVALID_HANDLE;

static bgfx::UniformHandle s_uTexColor = BGFX_INVALID_HANDLE;  // Sampler
static bgfx::UniformHandle s_uTexelSize = BGFX_INVALID_HANDLE; // Blur offsets
static bgfx::UniformHandle s_uTintColor = BGFX_INVALID_HANDLE; // Rect color

// Offscreen frame buffers
static bgfx::TextureHandle s_TexSceneColor = BGFX_INVALID_HANDLE;
static bgfx::FrameBufferHandle s_FbScene = BGFX_INVALID_HANDLE;

static bgfx::TextureHandle s_TexBlurA = BGFX_INVALID_HANDLE;
static bgfx::FrameBufferHandle s_FbBlurA = BGFX_INVALID_HANDLE;

static bgfx::TextureHandle s_TexBlurB = BGFX_INVALID_HANDLE;
static bgfx::FrameBufferHandle s_FbBlurB = BGFX_INVALID_HANDLE;

// For downsampling
static uint16_t s_SceneWidth = 0;
static uint16_t s_SceneHeight = 0;

// static rendering::PosColorVertex cubeVertices[] = {
//     {-1.0f, 1.0f, 1.0f, 0xff000000},   {1.0f, 1.0f, 1.0f, 0xff0000ff},
//     {-1.0f, -1.0f, 1.0f, 0xff00ff00},  {1.0f, -1.0f, 1.0f, 0xff00ffff},
//     {-1.0f, 1.0f, -1.0f, 0xffff0000},  {1.0f, 1.0f, -1.0f, 0xffff00ff},
//     {-1.0f, -1.0f, -1.0f, 0xffffff00}, {1.0f, -1.0f, -1.0f, 0xffffffff},
// };
//
// static const uint16_t cubeTriList[] = {
//     0, 1, 2, 1, 3, 2, 4, 6, 5, 5, 6, 7, 0, 2, 4, 4, 2, 6,
//     1, 5, 3, 5, 7, 3, 0, 4, 1, 4, 5, 1, 2, 3, 6, 6, 3, 7,
// };
int rendering::window_width = 800;
int rendering::window_height = 600;
Camera *rendering::main_camera = nullptr;
Camera rendering::game_camera{rendering::main_view};
int main(int argv, char **args) {
#ifdef _WIN32
  // search dll in ./lib
  SetDllDirectoryA("lib");
#endif
  // set QoS class for macOS, for best performance
#if TARGET_OS_OSX
  pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
#endif
  rendering::main_camera = &rendering::game_camera;
  sol::state lua;
  int x = 0;
  lua.set_function("beep", [&x] { ++x; });
  // call beep 100 times
  auto code = "beep()";
  lua.safe_script(code);

  assert(x == 1);
  SDL_Log("lua result: %d", x);
  SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
  SDL_SetHint(SDL_HINT_IME_SUPPORT_EXTENDED_TEXT, "1");
  // print bgfx version
  SDL_Log("bgfx version: %d OSX:%d", BGFX_API_VERSION, BX_PLATFORM_OSX);
  // print libsdl version
  SDL_version compiled;
  SDL_version linked;
  SDL_VERSION(&compiled);
  SDL_GetVersion(&linked);

  SDL_Log("SDL compile version: %d.%d.%d", static_cast<int>(compiled.major),
          static_cast<int>(compiled.minor), static_cast<int>(compiled.patch));
  SDL_Log("SDL link version: %d.%d.%d", static_cast<int>(linked.major),
          static_cast<int>(linked.minor), static_cast<int>(linked.patch));

#if TARGET_OS_OSX
  setSmoothScrolling(true);
#endif
  using std::cerr;
  using std::endl;

  if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
    cerr << "SDL_Init Error: " << SDL_GetError() << endl;
    return EXIT_FAILURE;
  }

  rendering::window_width = 1280;
  rendering::window_height = 720;
  SDL_Window *win = SDL_CreateWindow(
      "OpenBMS", 100, 100, rendering::window_width, rendering::window_height,
      SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
  // this is intended to get actual window size on mobile devices
  SDL_GetWindowSize(win, &rendering::window_width, &rendering::window_height);
  SDL_Log("Window size: %d x %d", rendering::window_width,
          rendering::window_height);
  if (win == nullptr) {
    cerr << "SDL_CreateWindow Error: " << SDL_GetError() << endl;
    return EXIT_FAILURE;
  }

  // this is intended; we don't need renderer for bgfx but SDL creates window
  // handler after renderer creation on iOS
#if TARGET_OS_IPHONE
  SDL_CreateRenderer(
      win, -1,
      SDL_RENDERER_ACCELERATED |
          SDL_RENDERER_PRESENTVSYNC); // Intentionally discarding return value
  SDL_SetWindowFullscreen(win, SDL_WINDOW_FULLSCREEN);
#endif
#if !BX_PLATFORM_EMSCRIPTEN
  SDL_SysWMinfo wmi;
  SDL_VERSION(&wmi.version);
  SDL_Log("SDL_major: %d, SDL_minor: %d, SDL_patch: %d\n", wmi.version.major,
          wmi.version.minor, wmi.version.patch);
  wmi.version.major = 2.0;
  wmi.version.minor = 0;
  if (!SDL_GetWindowWMInfo(win, &wmi)) {
    printf("SDL_SysWMinfo could not be retrieved. SDL_Error: %s\n",
           SDL_GetError());
    return 1;
  }
  bgfx::renderFrame(); // single threaded mode
#endif                 // !BX_PLATFORM_EMSCRIPTEN

  bgfx::PlatformData pd{};
  setup_bgfx_platform_data(pd, wmi);

  bgfx::Init bgfx_init;
  bgfx_init.type = bgfx::RendererType::Count; // auto choose renderer
  bgfx_init.resolution.width = rendering::window_width;
  bgfx_init.resolution.height = rendering::window_height;
  bgfx_init.resolution.reset = BGFX_RESET_MSAA_X2 | (TARGET_IS_IOS ? BGFX_RESET_VSYNC : 0);
  bgfx_init.platformData = pd;
  bgfx::init(bgfx_init);
  // bgfx::setDebug(BGFX_DEBUG_TEXT);

  // bgfx::setPlatformData(pd);

  run();
  bgfx::shutdown();

  SDL_DestroyWindow(win);
  SDL_Quit();
  SDL_Log("SDL quit");

  return EXIT_SUCCESS;
}

void run() {
  ApplicationContext context;
  bgfx::setViewMode(rendering::ui_view, bgfx::ViewMode::Sequential);
  SceneManager sceneManager(context);
  sceneManager.changeScene(new MainMenuScene(context));

  // SDL_RenderClear(ren);
  // SDL_RenderCopy(ren, tex, nullptr, nullptr);
  // SDL_RenderPresent(ren);
  SDL_Event e;

  auto lastFrameTime = std::chrono::high_resolution_clock::now();

  // Initialize bgfx
  rendering::PosColorVertex::init();
  rendering::PosTexVertex::init();
  rendering::PosTexCoord0Vertex::init();
  createFrameBuffers(rendering::window_width, rendering::window_height);

  s_ProgBlurH = rendering::ShaderManager::getInstance().getProgram(
      "blur/vs_blur.bin", "blur/fs_blurH.bin");
  s_ProgBlurV = rendering::ShaderManager::getInstance().getProgram(
      "blur/vs_blur.bin", "blur/fs_blurV.bin");
  s_ProgRect = rendering::ShaderManager::getInstance().getProgram(
      "blur/vs_blur.bin", "fs_rect_tint.bin");
  s_uTexColor = bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);
  s_uTexelSize = bgfx::createUniform("u_texelSize", bgfx::UniformType::Vec4);
  s_uTintColor = bgfx::createUniform("u_tintColor", bgfx::UniformType::Vec4);

  // We will use this to reference where we're drawing
  // This is set once to determine the clear color to use on starting a new
  // frame
  bgfx::setViewClear(rendering::clear_view, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
                     0x00000000);
  bgfx::setViewClear(rendering::ui_view, BGFX_CLEAR_DEPTH, 0x00000000);
  bgfx::setViewClear(rendering::bga_view, BGFX_CLEAR_COLOR, 0x00000000);
  bgfx::setViewClear(rendering::bga_layer_view, BGFX_CLEAR_NONE, 0x00000000);

  bgfx::setViewClear(rendering::main_view, BGFX_CLEAR_DEPTH, 0x00000000, 1.0f,
                     0);
  bgfx::setViewClear(rendering::blur_view_h, BGFX_CLEAR_COLOR, 0x00000000, 1.0f,
                     0);
  bgfx::setViewClear(rendering::blur_view_v, BGFX_CLEAR_COLOR, 0x00000000, 1.0f,
                     0);
  bgfx::setViewClear(rendering::final_view, BGFX_CLEAR_COLOR, 0x00000000, 1.0f,
                     0);

  // This is set to determine the size of the drawable surface
  bgfx::setViewRect(rendering::ui_view, 0, 0, rendering::window_width,
                    rendering::window_height);
  bgfx::setViewRect(rendering::bga_view, 0, 0, rendering::window_width,
                    rendering::window_height);
  bgfx::setViewRect(rendering::bga_layer_view, 0, 0, rendering::window_width,
                    rendering::window_height);
  auto program =
      rendering::ShaderManager::getInstance().getProgram(SHADER_SIMPLE);
  resetViewTransform();

  TextView fpsText("assets/fonts/notosanscjkjp.ttf", 24);
  while (!context.quitFlag) {

    auto currentFrameTime = std::chrono::high_resolution_clock::now();
    float deltaTime =
        std::chrono::duration<float, std::chrono::seconds::period>(
            currentFrameTime - lastFrameTime)
            .count();
    lastFrameTime = currentFrameTime;

    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT) {
        context.quitFlag = true;
      }
      auto result = sceneManager.handleEvents(e);
      if (result.quit) {
        context.quitFlag = true;
      }

      // on window resize
      if (e.type == SDL_WINDOWEVENT &&
          e.window.event == SDL_WINDOWEVENT_RESIZED) {
        rendering::window_width = e.window.data1;
        rendering::window_height = e.window.data2;

        // set bgfx resolution
        bgfx::reset(rendering::window_width, rendering::window_height,
                    BGFX_RESET_MSAA_X2 | (TARGET_IS_IOS ? BGFX_RESET_VSYNC : 0));
        SDL_Log("Window size: %d x %d", rendering::window_width,
                rendering::window_height);
        destroyFrameBuffers();
        createFrameBuffers(rendering::window_width, rendering::window_height);
        resetViewTransform();
      }
    }
    sceneManager.update(deltaTime);

    //    bgfx::reset(rendering::window_width, rendering::window_height);
    // SDL_Log("Window size: %d x %d", rendering::window_width,
    //         rendering::window_height);
    // clear color

    bgfx::touch(rendering::clear_view);
    bgfx::submit(rendering::clear_view, program);
    bgfx::touch(rendering::ui_view);
    bgfx::touch(rendering::bga_view);
    bgfx::touch(rendering::bga_layer_view);
    bgfx::touch(rendering::final_view);
    bgfx::touch(rendering::blur_view_h);
    bgfx::touch(rendering::blur_view_v);
    bgfx::submit(rendering::clear_view, program);

    sceneManager.render();
    blurHorizontal();
    blurVertical();
    drawFinal(rendering::window_width, rendering::window_height);

    // render fps, rounded to 2 decimal places
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << 1.0f / deltaTime;
    fpsText.setText(oss.str());
    fpsText.setPosition(10, 10);
    RenderContext renderContext;
    fpsText.render(renderContext);
    // shift left by 1
    // float translate[16];
    // bx::mtxTranslate(translate, 200.0f, 500.0f, 0.0f);
    // float rotate[16];
    // bx::mtxRotateZ(rotate, bx::toRad(45.0f));
    // float mtx[16];
    // bx::mtxMul(mtx, rotate, translate);
    // bgfx::setTransform(mtx);
    //
    // bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
    //
    // bgfx::setVertexBuffer(0, triangleVbh);
    // bgfx::setIndexBuffer(triangleIbh);
    // bgfx::submit(rendering::ui_view, program);
    //
    // bx::mtxTranslate(translate, 300.0f, 500.0f, 0.0f);
    // bx::mtxRotateZ(rotate, bx::toRad(45.0f));
    // bx::mtxMul(mtx, rotate, translate);
    // bgfx::setTransform(mtx);
    // bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
    // bgfx::setVertexBuffer(0, rectVbh);
    // bgfx::setIndexBuffer(rectIbh);
    // bgfx::submit(rendering::ui_view, program);

    // draw cube
    //    bgfx::touch(rendering::main_view);
    //
    //    bgfx::setVertexBuffer(0, vbh);
    //    bgfx::setIndexBuffer(ibh);
    //    bgfx::setState(BGFX_STATE_DEFAULT);
    //    bgfx::submit(rendering::main_view, program);

    bgfx::frame();
    sceneManager.handleDeferred();
    context.currentFrame++;
    //
  }
  sceneManager.cleanup();
  // bgfx::destroy(vbh);
  // bgfx::destroy(ibh);
}

void resetViewTransform() {
  float ortho[16];
  bx::mtxOrtho(ortho, 0.0f, rendering::window_width, rendering::window_height,
               0.0f, 0.0f, 100.0f, 0.0f, bgfx::getCaps()->homogeneousDepth);

  bgfx::setViewTransform(rendering::ui_view, nullptr, ortho);
  bgfx::setViewRect(rendering::ui_view, 0, 0, rendering::window_width,
                    rendering::window_height);
  bgfx::setViewTransform(rendering::bga_view, nullptr, ortho);
  bgfx::setViewRect(rendering::bga_view, 0, 0, rendering::window_width,
                    rendering::window_height);
  bgfx::setViewTransform(rendering::bga_layer_view, nullptr, ortho);
  bgfx::setViewRect(rendering::bga_layer_view, 0, 0, rendering::window_width,
                    rendering::window_height);
  bgfx::setViewTransform(rendering::clear_view, nullptr, ortho);
  bgfx::setViewRect(rendering::clear_view, 0, 0, rendering::window_width,
                    rendering::window_height);
  bgfx::setViewTransform(rendering::final_view, nullptr, ortho);
  bgfx::setViewRect(rendering::final_view, 0, 0, rendering::window_width,
                    rendering::window_height);
  bgfx::setViewTransform(rendering::blur_view_h, nullptr, ortho);
  bgfx::setViewTransform(rendering::blur_view_v, nullptr, ortho);

  bx::Vec3 at = {4.0f, 2.0f, 0.0f};
  bx::Vec3 eye = {4.0f, 1.5f, -2.1f};

  float aspect =
      float(rendering::window_width) / float(rendering::window_height);
  rendering::game_camera.edit()
      .setPosition(eye)
      .setLookAt(at)
      .setAspectRatio(aspect)
      .setViewRect(0, 0, rendering::window_width, rendering::window_height)
      .commit();
  if (rendering::main_camera != nullptr) {
    rendering::main_camera->render();
  }
}

void createFrameBuffers(uint16_t windowW, uint16_t windowH) {
  // Decide on a half-resolution for blur to improve performance
  s_SceneWidth = windowW / 2;
  s_SceneHeight = windowH / 2;
  if (s_SceneWidth < 1)
    s_SceneWidth = 1;
  if (s_SceneHeight < 1)
    s_SceneHeight = 1;

  // Scene color
  s_TexSceneColor =
      bgfx::createTexture2D(s_SceneWidth, s_SceneHeight, false, 1,
                            bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_RT);
  s_FbScene = bgfx::createFrameBuffer(1, &s_TexSceneColor, true);

  // Blur intermediate A
  s_TexBlurA =
      bgfx::createTexture2D(s_SceneWidth, s_SceneHeight, false, 1,
                            bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_RT);
  s_FbBlurA = bgfx::createFrameBuffer(1, &s_TexBlurA, true);

  // Blur intermediate B
  s_TexBlurB =
      bgfx::createTexture2D(s_SceneWidth, s_SceneHeight, false, 1,
                            bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_RT);
  s_FbBlurB = bgfx::createFrameBuffer(1, &s_TexBlurB, true);
  bgfx::setViewFrameBuffer(rendering::bga_view, s_FbScene);
  bgfx::setViewFrameBuffer(rendering::bga_layer_view, s_FbScene);
}

void destroyFrameBuffers() {
  if (bgfx::isValid(s_FbScene))
    bgfx::destroy(s_FbScene);
  if (bgfx::isValid(s_FbBlurA))
    bgfx::destroy(s_FbBlurA);
  if (bgfx::isValid(s_FbBlurB))
    bgfx::destroy(s_FbBlurB);

  if (bgfx::isValid(s_TexSceneColor))
    bgfx::destroy(s_TexSceneColor);
  if (bgfx::isValid(s_TexBlurA))
    bgfx::destroy(s_TexBlurA);
  if (bgfx::isValid(s_TexBlurB))
    bgfx::destroy(s_TexBlurB);

  s_FbScene = BGFX_INVALID_HANDLE;
  s_FbBlurA = BGFX_INVALID_HANDLE;
  s_FbBlurB = BGFX_INVALID_HANDLE;
  s_TexSceneColor = BGFX_INVALID_HANDLE;
  s_TexBlurA = BGFX_INVALID_HANDLE;
  s_TexBlurB = BGFX_INVALID_HANDLE;
}

void blurHorizontal() {
  bgfx::setViewFrameBuffer(rendering::blur_view_h, s_FbBlurA);

  bgfx::setViewRect(rendering::blur_view_h, 0, 0, s_SceneWidth, s_SceneHeight);

  // Bind scene color
  bgfx::setTexture(0, s_uTexColor, s_TexSceneColor);

  // Full-screen quad
  rendering::screenSpaceQuad();

  // Set uniform for texel size
  float texelSize[4];
  texelSize[0] = 1.0f / float(s_SceneWidth);  // offset in X
  texelSize[1] = 1.0f / float(s_SceneHeight); // offset in Y
  texelSize[2] = 0.0f;
  texelSize[3] = 0.0f;
  bgfx::setUniform(s_uTexelSize, texelSize);
  bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
  bgfx::submit(rendering::blur_view_h, s_ProgBlurH);
}

void blurVertical() {
  bgfx::setViewFrameBuffer(rendering::blur_view_v, s_FbBlurB);
  bgfx::setViewRect(rendering::blur_view_v, 0, 0, s_SceneWidth, s_SceneHeight);

  // Bind horizontally blurred texture
  bgfx::setTexture(0, s_uTexColor, s_TexBlurA);

  // Full-screen quad
  rendering::screenSpaceQuad();

  float texelSize[4];
  texelSize[0] = 1.0f / float(s_SceneWidth);
  texelSize[1] = 1.0f / float(s_SceneHeight);
  texelSize[2] = 0.0f;
  texelSize[3] = 0.0f;
  bgfx::setUniform(s_uTexelSize, texelSize);
  bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
  bgfx::submit(rendering::blur_view_v, s_ProgBlurV);
}

void drawFinal(uint16_t windowW, uint16_t windowH) {
  bgfx::setViewFrameBuffer(rendering::final_view, BGFX_INVALID_HANDLE);
  bgfx::setViewRect(rendering::final_view, 0, 0, windowW, windowH);
  // float ortho[16];
  // bx::mtxOrtho(ortho, 0.0f, rendering::window_width,
  // rendering::window_height,
  //              0.0f, 0.0f, 100.0f, 0.0f, bgfx::getCaps()->homogeneousDepth);
  // bgfx::setViewTransform(rendering::final_view, nullptr, ortho);

  // 1) (Optional) draw normal UI, 2D overlays, etc. here with separate calls.
  // For demonstration, we do nothing. Just keep the backbuffer as-is.

  // 2) Draw a "frosted" rectangle in the center.
  // We'll cheat by reusing the same full-screen quad. Instead, let's create a
  // scissor rect to limit where we draw. (In real code, you'd create a smaller
  // screen-space quad or use a transform.)

  // // Example scissor: a rectangle in the center
  // int rectW = int(windowW * 0.4f);
  // int rectH = int(windowH * 0.4f);
  // int rectX = (windowW - rectW) / 2;
  // int rectY = (windowH - rectH) / 2;
  // bgfx::setScissor(uint16_t(rectX), uint16_t(rectY), uint16_t(rectW),
  //                  uint16_t(rectH));

  // We'll sample the blurred texture. Because we downsampled, we need to adjust
  // UVs if we want it to align with the actual screen. A simple approach is to
  // just fill it with the entire blurred image, or compute the correct UV
  // scale. For demonstration, let's just fill with the entire blurred texture.

  bgfx::setTexture(0, s_uTexColor, s_TexBlurB);

  // Frosted tint
  float tintColor[4] = {1.0f, 1.0f, 1.0f, 0.60f}; // 60% alpha
  bgfx::setUniform(s_uTintColor, tintColor);
  bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
  // Full-screen quad geometry (but scissored now)
  rendering::screenSpaceQuad();
  bgfx::submit(rendering::final_view, s_ProgRect);

  // Disable scissor for subsequent draws
  // bgfx::setScissor();
}
