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
#include "video/VLCInstance.h"
#include "view/TextView.h"
#include <vlcpp/vlc.hpp>
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
bgfx::VertexLayout rendering::PosColorVertex::ms_decl;
bgfx::VertexLayout rendering::PosTexVertex::ms_decl;
bgfx::VertexLayout rendering::PosTexCoord0Vertex::ms_decl;

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
int main(int argv, char **args) {
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
  const char *runPath = args[0];
  std::string plugin_path = runPath;
  plugin_path = plugin_path.substr(0, plugin_path.find_last_of('/'));
  plugin_path += "/plugins";
  setenv("VLC_PLUGIN_PATH", plugin_path.c_str(), 1);
  SDL_Log("VLC_PLUGIN_PATH: %s", getenv("VLC_PLUGIN_PATH"));
#endif
  using std::cerr;
  using std::endl;

  // print libvlc version
  SDL_Log("libvlc version: %s", libvlc_get_version());
  // vlc instance
  SDL_Log("VLC init...");
  VLCInstance::getInstance();

  SDL_Log("VLC init done");
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
  ren = SDL_CreateRenderer(
      win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
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
  bgfx_init.resolution.reset = BGFX_RESET_VSYNC | BGFX_RESET_MSAA_X2;
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
  bgfx::setViewMode(0, bgfx::ViewMode::Sequential);
  SceneManager sceneManager(context);
  sceneManager.changeScene(new MainMenuScene(context));

  // SDL_RenderClear(ren);
  // SDL_RenderCopy(ren, tex, nullptr, nullptr);
  // SDL_RenderPresent(ren);
  SDL_Event e;
  bool quit = false;
  auto lastFrameTime = std::chrono::high_resolution_clock::now();

  // Initialize bgfx
  rendering::PosColorVertex::init();
  rendering::PosTexVertex::init();
  rendering::PosTexCoord0Vertex::init();

  // const bgfx::VertexBufferHandle vbh = bgfx::createVertexBuffer(
  //     bgfx::makeRef(cubeVertices, sizeof(cubeVertices)),
  //     rendering::PosColorVertex::ms_decl);
  //
  // // Create index buffer
  // const bgfx::IndexBufferHandle ibh =
  //     bgfx::createIndexBuffer(bgfx::makeRef(cubeTriList,
  //     sizeof(cubeTriList)));
  // rendering::PosColorVertex triangleVert[] = {
  //     {-100.0f, -100.0f, 0.0f, 0x339933FF},
  //     {100.0f, -100.0f, 0.0f, 0x993333FF},
  //     {0.0f, 100.0f, 0.0f, 0x333399FF},
  // };
  // constexpr uint16_t triangleInd[] = {0, 1, 2};

  // bgfx::VertexBufferHandle triangleVbh = bgfx::createVertexBuffer(
  //     bgfx::makeRef(triangleVert, sizeof(triangleVert)),
  //     rendering::PosColorVertex::ms_decl);
  // bgfx::IndexBufferHandle triangleIbh =
  //     bgfx::createIndexBuffer(bgfx::makeRef(triangleInd,
  //     sizeof(triangleInd)));
  // rendering::PosColorVertex rectVert[] = {
  //     {-100.0f, -100.0f, 100.0f, 0x339933FF},
  //     {100.0f, -100.0f, 0.0f, 0x993333FF},
  //     {100.0f, 100.0f, 0.0f, 0x333399FF},
  //     {-100.0f, 100.0f, 0.0f, 0x993399FF},
  // };
  // uint16_t rectInd[] = {0, 1, 2, 0, 2, 3};
  // bgfx::VertexBufferHandle rectVbh =
  //     bgfx::createVertexBuffer(bgfx::makeRef(rectVert, sizeof(rectVert)),
  //                              rendering::PosColorVertex::ms_decl);
  // bgfx::IndexBufferHandle rectIbh =
  //     bgfx::createIndexBuffer(bgfx::makeRef(rectInd, sizeof(rectInd)));

  // We will use this to reference where we're drawing
  // This is set once to determine the clear color to use on starting a new
  // frame
  bgfx::setViewClear(rendering::clear_view, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
                     0x00000000);
  bgfx::setViewClear(rendering::ui_view, BGFX_CLEAR_DEPTH, 0x00000000);
  bgfx::setViewClear(rendering::bga_view, BGFX_CLEAR_DEPTH, 0x00000000);
  bgfx::setViewClear(rendering::bga_layer_view, BGFX_CLEAR_DEPTH, 0x00000000);
  bgfx::setViewClear(rendering::main_view, BGFX_CLEAR_DEPTH, 0x00000000, 1.0f,
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

  //  VideoPlayer videoPlayer;
  //  videoPlayer.loadVideo("assets/video/sample.mp4");
  //  videoPlayer.viewWidth = rendering::window_width;
  //  videoPlayer.viewHeight = rendering::window_height;
  //  videoPlayer.play();
  resetViewTransform();
  TextView fpsText("assets/fonts/notosanscjkjp.ttf", 24);
  while (!quit) {

    // SDL_RenderCopy(ren, tex, nullptr, nullptr);
    auto currentFrameTime = std::chrono::high_resolution_clock::now();
    float deltaTime =
        std::chrono::duration<float, std::chrono::seconds::period>(
            currentFrameTime - lastFrameTime)
            .count();
    lastFrameTime = currentFrameTime;

    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT) {
        quit = true;
      }
      auto result = sceneManager.handleEvents(e);
      if (result.quit) {
        quit = true;
      }

      // on window resize
      if (e.type == SDL_WINDOWEVENT &&
          e.window.event == SDL_WINDOWEVENT_RESIZED) {
        rendering::window_width = e.window.data1;
        rendering::window_height = e.window.data2;

        // set bgfx resolution
        bgfx::reset(rendering::window_width, rendering::window_height,
                    BGFX_RESET_VSYNC | BGFX_RESET_MSAA_X2);
        SDL_Log("Window size: %d x %d", rendering::window_width,
                rendering::window_height);
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

    bgfx::submit(rendering::clear_view, program);
    sceneManager.render();
    // render fps, rounded to 2 decimal places
    // std::ostringstream oss;
    // oss << std::fixed << std::setprecision(2) << 1.0f / deltaTime;
    // fpsText.setText(oss.str());
    // fpsText.setPosition(10, 10);
    // RenderContext renderContext;
    // fpsText.render(renderContext);
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

  bx::Vec3 at = {4.0f, 2.0f, 0.0f};
  bx::Vec3 eye = {4.0f, 1.5f, -2.1f};
  float viewMtx[16];
  bx::mtxLookAt(viewMtx, eye, at);
  float projMtx[16];
  bx::mtxProj(projMtx, 120.0f,
              float(rendering::window_width) / float(rendering::window_height),
              0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);
  bgfx::setViewTransform(rendering::main_view, viewMtx, projMtx);
  bgfx::setViewRect(rendering::main_view, 0, 0, rendering::window_width,
                    rendering::window_height);
}