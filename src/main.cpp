#define BX_CONFIG_DEBUG 1

#include "bx/math.h"
#include <cstdio>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_video.h>
#include "targets.h"
#include "main.h"
#include "scene/MainMenuScene.h"
#include "scene/SceneManager.h"
#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>
#include "tinyfiledialogs.h"
#include "Utils.h"
#include <bgfx/bgfx.h>
#include <bgfx/embedded_shader.h>
#include <bgfx/platform.h>
#include <bx/platform.h>
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
class MainFunctionRAII {
private:
  std::atomic<bool> &quitFlag;

public:
  explicit MainFunctionRAII(std::atomic<bool> &flag) : quitFlag(flag) {
    quitFlag = false;
  }

  ~MainFunctionRAII() {
    quitFlag = true;
    std::cout << "Main function is quitting..." << std::endl;
  }
};

struct PosColorVertex {
  static bgfx::VertexLayout ms_decl;
  float x;
  float y;
  float z;
  uint32_t abgr;
};

struct PosColorVertex2D {
  static bgfx::VertexLayout ms_decl;
  float x;
  float y;
  uint32_t abgr;
};

bgfx::VertexLayout PosColorVertex::ms_decl;
bgfx::VertexLayout PosColorVertex2D::ms_decl;
static PosColorVertex cubeVertices[] = {
    {-1.0f, 1.0f, 1.0f, 0xff000000},   {1.0f, 1.0f, 1.0f, 0xff0000ff},
    {-1.0f, -1.0f, 1.0f, 0xff00ff00},  {1.0f, -1.0f, 1.0f, 0xff00ffff},
    {-1.0f, 1.0f, -1.0f, 0xffff0000},  {1.0f, 1.0f, -1.0f, 0xffff00ff},
    {-1.0f, -1.0f, -1.0f, 0xffffff00}, {1.0f, -1.0f, -1.0f, 0xffffffff},
};

static const uint16_t cubeTriList[] = {
    0, 1, 2, 1, 3, 2, 4, 6, 5, 5, 6, 7, 0, 2, 4, 4, 2, 6,
    1, 5, 3, 5, 7, 3, 0, 4, 1, 4, 5, 1, 2, 3, 6, 6, 3, 7,
};
bgfx::ShaderHandle loadShader(const char *FILENAME) {
  const char *shaderPath = "???";
  switch (bgfx::getRendererType()) {
  case bgfx::RendererType::Noop:
  case bgfx::RendererType::Direct3D11:
  case bgfx::RendererType::Direct3D12:
    shaderPath = "./shaders/dx11/";
    break;
  case bgfx::RendererType::Gnm:
    shaderPath = "./shaders/pssl/";
    break;
  case bgfx::RendererType::Metal:
    shaderPath = "./shaders/metal/";
    break;
  case bgfx::RendererType::OpenGL:
    shaderPath = "./shaders/glsl/";
    break;
  case bgfx::RendererType::OpenGLES:
    shaderPath = "./shaders/essl/";
    break;
  case bgfx::RendererType::Vulkan:
    shaderPath = "./shaders/spirv/";
    break;
  default:
    throw std::runtime_error("Unknown renderer type");
  }

  size_t shaderLen = strlen(shaderPath);
  size_t fileLen = strlen(FILENAME);
  char *filePath = (char *)malloc(shaderLen + fileLen);
  memcpy(filePath, shaderPath, shaderLen);
  memcpy(&filePath[shaderLen], FILENAME, fileLen);
  SDL_Log("Loading %s", filePath);

  FILE *file = fopen(filePath, "rb");
  if (!file) {
    perror("file open error");
    throw std::runtime_error("no such file");
  }
  fseek(file, 0, SEEK_END);
  long fileSize = ftell(file);
  fseek(file, 0, SEEK_SET);

  const bgfx::Memory *mem = bgfx::alloc(fileSize + 1);
  fread(mem->data, 1, fileSize, file);
  mem->data[mem->size - 1] = '\0';
  fclose(file);

  return bgfx::createShader(mem);
}

int main(int argv, char **args) {

  // print bgfx version
  std::cout << "bgfx version: " << BGFX_API_VERSION << "OSX:" << BX_PLATFORM_OSX
            << std::endl;
  // print libsdl version
  SDL_version compiled;
  SDL_version linked;
  SDL_VERSION(&compiled);
  SDL_GetVersion(&linked);
  std::cout << "TARGET_OS_OSX: " << TARGET_OS_OSX << std::endl;
  std::cout << "TARGET_OS_IPHONE: " << TARGET_OS_IPHONE << std::endl;
  std::cout << "TARGET_OS_IPHONE_SIMULATOR: " << TARGET_IPHONE_SIMULATOR
            << std::endl;
  std::cout << "TARGET_OS_IOS: " << TARGET_OS_IOS << std::endl;

  std::cout << "SDL compile version: " << static_cast<int>(compiled.major)
            << "." << static_cast<int>(compiled.minor) << "."
            << static_cast<int>(compiled.patch) << std::endl;
  std::cout << "SDL link version: " << static_cast<int>(linked.major) << "."
            << static_cast<int>(linked.minor) << "."
            << static_cast<int>(linked.patch) << std::endl;
  std::atomic<bool> quitFlag(false);

#if TARGET_OS_OSX
  setSmoothScrolling(true);
  const char *runpath = args[0];
  std::string plugin_path = runpath;
  plugin_path = plugin_path.substr(0, plugin_path.find_last_of("/"));
  plugin_path += "/plugins";
  setenv("VLC_PLUGIN_PATH", plugin_path.c_str(), 1);
  std::cout << "VLC_PLUGIN_PATH: " << getenv("VLC_PLUGIN_PATH") << std::endl;
#endif
  using std::cerr;
  using std::endl;
  auto dbHelper = ChartDBHelper::GetInstance();
  auto db = dbHelper.Connect();
  dbHelper.CreateChartMetaTable(db);
  dbHelper.CreateEntriesTable(db);
  auto entries = dbHelper.SelectAllEntries(db);
  // open folder select if no entries
  if (entries.empty()) {
#if TARGET_OS_IOS
    auto path = Utils::GetDocumentsPath("BMS");
    std::filesystem::create_directories(path);
    entries.push_back(path);
    dbHelper.InsertEntry(db, path);
#else
    char *folder_c = tinyfd_selectFolderDialog("Select Folder", nullptr);
    std::string folder;
    if (folder_c == nullptr) {
      std::cerr << "tinyfd_selectFolderDialog error: " << strerror(errno)
                << std::endl;
      // get input from stdin
      std::cout << "Failed to open folder select dialog.\n";
      while (folder.empty()) {
        std::cout << "Enter bms folder path: ";
        std::cin >> folder;
        if (folder.empty()) {
          continue;
        }
        // replace ~ with home directory
        if (folder[0] == '~') {
          folder.replace(0, 1, getenv("HOME"));
        }
        std::ifstream test(folder);
        if (!test)
          folder = "";
      }
    } else {
      folder = folder_c;
    }
    std::filesystem::path path(folder);
    dbHelper.InsertEntry(db, path);
#endif
  }
  std::thread t1(LoadCharts, std::ref(dbHelper), std::ref(db),
                 std::ref(quitFlag));
  threadRAII t1_raii(std::move(t1));
  MainFunctionRAII mainFunctionRAII(quitFlag);
  // vlc instance
  std::cout << "VLC init..." << std::endl;
  // auto instance = VLC::Instance(0, nullptr);

  std::cout << "VLC init done" << std::endl;
  if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
    cerr << "SDL_Init Error: " << SDL_GetError() << endl;
    return EXIT_FAILURE;
  }

  SceneManager sceneManager;
  int width = 800;
  int height = 600;
  SDL_Window *win = SDL_CreateWindow("Hello World!", 100, 100, width, height,
                                     SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
  // this is intended to get actual window size on mobile devices
  SDL_GetWindowSize(win, &width, &height);
  SDL_Log("Window size: %d x %d", width, height);
  if (win == nullptr) {
    cerr << "SDL_CreateWindow Error: " << SDL_GetError() << endl;
    return EXIT_FAILURE;
  }
  SDL_Renderer *ren = SDL_CreateRenderer(
      win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (ren == nullptr) {
    cerr << "SDL_CreateRenderer Error" << SDL_GetError() << endl;
    bgfx::shutdown();
    SDL_DestroyWindow(win);
    SDL_Quit();
    return EXIT_FAILURE;
  }
#if !BX_PLATFORM_EMSCRIPTEN
  SDL_SysWMinfo wmi;
  SDL_VERSION(&wmi.version);
  if (!SDL_GetWindowWMInfo(win, &wmi)) {
    printf("SDL_SysWMinfo could not be retrieved. SDL_Error: %s\n",
           SDL_GetError());
    return 1;
  }
  bgfx::renderFrame(); // single threaded mode
#endif                 // !BX_PLATFORM_EMSCRIPTEN

  bgfx::PlatformData pd{};
#if BX_PLATFORM_WINDOWS
  pd.nwh = wmi.info.win.window;
#elif BX_PLATFORM_OSX
  pd.nwh = wmi.info.cocoa.window;
#elif BX_PLATFORM_LINUX
  pd.ndt = wmi.info.x11.display;
  pd.nwh = (void *)(uintptr_t)wmi.info.x11.window;
#elif BX_PLATFORM_EMSCRIPTEN
  pd.nwh = (void *)"#canvas";
#elif BX_PLATFORM_IOS
  pd.ndt = nullptr;
  pd.nwh = GetIOSWindowHandle(wmi.info.uikit.window);
  pd.context = nullptr;
  pd.backBuffer = nullptr;
  pd.backBufferDS = nullptr;
#endif // BX_PLATFORM_WINDOWS ? BX_PLATFORM_OSX ? BX_PLATFORM_LINUX ?
       // BX_PLATFORM_EMSCRIPTEN
  bgfx::Init bgfx_init;
  bgfx_init.type = bgfx::RendererType::Count; // auto choose renderer
  bgfx_init.resolution.width = width;
  bgfx_init.resolution.height = height;
  bgfx_init.resolution.reset = BGFX_RESET_VSYNC;
  bgfx_init.platformData = pd;
  bgfx::init(bgfx_init);
  bgfx::setDebug(BGFX_DEBUG_TEXT);
  // bgfx::setPlatformData(pd);

  bgfx::setViewMode(0, bgfx::ViewMode::Sequential);

  sceneManager.changeScene(new MainMenuScene(ren));

  SDL_Surface *bmp = SDL_LoadBMP("./assets/img/sdl.bmp");
  if (bmp == nullptr) {
    cerr << "SDL_LoadBMP Error: " << SDL_GetError() << endl;
    SDL_DestroyRenderer(ren);
    bgfx::shutdown();
    SDL_DestroyWindow(win);
    SDL_Quit();
    return EXIT_FAILURE;
  }

  SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, bmp);

  if (tex == nullptr) {
    cerr << "SDL_CreateTextureFromSurface Error: " << SDL_GetError() << endl;
    SDL_FreeSurface(bmp);
    bgfx::shutdown();
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return EXIT_FAILURE;
  }
  SDL_FreeSurface(bmp);

  // SDL_RenderClear(ren);
  // SDL_RenderCopy(ren, tex, nullptr, nullptr);
  // SDL_RenderPresent(ren);
  SDL_Event e;
  bool quit = false;
  auto lastFrameTime = std::chrono::high_resolution_clock::now();

  // Create vertex buffer
  PosColorVertex::ms_decl.begin()
      .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
      .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
      .end();
  PosColorVertex2D::ms_decl.begin()
      .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
      .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
      .end();
  bgfx::VertexBufferHandle vbh = bgfx::createVertexBuffer(
      bgfx::makeRef(cubeVertices, sizeof(cubeVertices)),
      PosColorVertex::ms_decl);

  // Create index buffer
  bgfx::IndexBufferHandle ibh =
      bgfx::createIndexBuffer(bgfx::makeRef(cubeTriList, sizeof(cubeTriList)));
  PosColorVertex triangleVert[] = {
      {-100.0f, -100.0f, 0.0f, 0x339933FF},
      {100.0f, -100.0f, 0.0f, 0x993333FF},
      {0.0f, 100.0f, 0.0f, 0x333399FF},
  };
  uint16_t triangleInd[] = {0, 1, 2};

  PosColorVertex rectVert[] = {
      {-100.0f, -100.0f, 0.0f, 0x339933FF},
      {100.0f, -100.0f, 0.0f, 0x993333FF},
      {100.0f, 100.0f, 0.0f, 0x333399FF},
      {-100.0f, 100.0f, 0.0f, 0x993399FF},
  };
  uint16_t rectInd[] = {0, 1, 2, 0, 2, 3};

  bgfx::VertexBufferHandle triangleVbh = bgfx::createVertexBuffer(
      bgfx::makeRef(triangleVert, sizeof(triangleVert)),
      PosColorVertex::ms_decl);
  bgfx::IndexBufferHandle triangleIbh =
      bgfx::createIndexBuffer(bgfx::makeRef(triangleInd, sizeof(triangleInd)));
  bgfx::VertexBufferHandle rectVbh = bgfx::createVertexBuffer(
      bgfx::makeRef(rectVert, sizeof(rectVert)), PosColorVertex::ms_decl);
  bgfx::IndexBufferHandle rectIbh =
      bgfx::createIndexBuffer(bgfx::makeRef(rectInd, sizeof(rectInd)));
  auto vs_handle = loadShader("vs_simple.bin");
  auto fs_handle = loadShader("fs_simple.bin");
  auto vs_ui_handle = loadShader("vs_ui.bin");
  auto fs_ui_handle = loadShader("fs_ui.bin");

  // We will use this to reference where we're drawing
  const bgfx::ViewId main_view_id = 1;
  const bgfx::ViewId secondary_view_id = 0;
  // This is set once to determine the clear color to use on starting a new
  // frame
  bgfx::setViewClear(main_view_id, BGFX_CLEAR_DEPTH,
                     0x303030ff); // Clear to dark gray
  bgfx::setViewClear(secondary_view_id, BGFX_CLEAR_DEPTH | BGFX_CLEAR_COLOR,
                     0x202020ff, 1.0f, 0);
  // This is set to determine the size of the drawable surface
  bgfx::setViewRect(main_view_id, 0, 0, width, height);
  auto program = bgfx::createProgram(vs_handle, fs_handle, true);
  auto program_ui = bgfx::createProgram(vs_ui_handle, fs_ui_handle, true);

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
    }
    sceneManager.update(deltaTime);
    // sceneManager.render(ren);
    bgfx::reset(width, height);
    bgfx::setViewRect(main_view_id, 0, 0, bgfx::BackbufferRatio::Equal);
    bgfx::touch(main_view_id);
    // ortho
    float ortho[16];
    bx::mtxOrtho(ortho, 0.0f, width, height, 0.0f, 0.0f, 100.0f, 0.0f,
                 bgfx::getCaps()->homogeneousDepth);
    // shift left by 1
    float translate[16];
    bx::mtxTranslate(translate, 200.0f, 500.0f, 0.0f);
    float rotate[16];
    bx::mtxRotateZ(rotate, bx::toRad(45.0f));
    float mtx[16];
    bx::mtxMul(mtx, rotate, translate);
    bgfx::setTransform(mtx);
    bgfx::setViewTransform(main_view_id, nullptr, ortho);
    bgfx::setViewRect(main_view_id, 0, 0, width, height);
    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);

    bgfx::setVertexBuffer(0, triangleVbh);
    bgfx::setIndexBuffer(triangleIbh);
    bgfx::submit(main_view_id, program);

    bx::mtxTranslate(translate, 300.0f, 500.0f, 0.0f);
    bx::mtxRotateZ(rotate, bx::toRad(45.0f));
    bx::mtxMul(mtx, rotate, translate);
    bgfx::setTransform(mtx);
    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
    bgfx::setVertexBuffer(0, rectVbh);
    bgfx::setIndexBuffer(rectIbh);
    bgfx::submit(main_view_id, program);
    // bgfx::frame();

    // draw cube
    // bgfx::touch(secondary_view_id);
    bx::Vec3 at = {0.0f, 0.0f, 0.0f};
    bx::Vec3 eye = {0.0f, 2.0f, -5.0f};
    float viewMtx[16];
    bx::mtxLookAt(viewMtx, eye, at);
    float projMtx[16];
    bx::mtxProj(projMtx, 60.0f, float(width) / float(height), 0.1f, 100.0f,
                bgfx::getCaps()->homogeneousDepth);
    bgfx::setViewTransform(secondary_view_id, viewMtx, projMtx);
    bgfx::setViewRect(secondary_view_id, 0, 0, width, height);
    bgfx::setVertexBuffer(0, vbh);
    bgfx::setIndexBuffer(ibh);
    bgfx::setState(BGFX_STATE_DEFAULT);
    bgfx::submit(secondary_view_id, program);

    bgfx::frame();
  }

  SDL_DestroyTexture(tex);
  SDL_DestroyRenderer(ren);
  bgfx::shutdown();
  SDL_DestroyWindow(win);
  SDL_Quit();
  std::cout << "SDL quit" << std::endl;

  return EXIT_SUCCESS;
}

void LoadCharts(ChartDBHelper &dbHelper, sqlite3 *db,
                std::atomic_bool &isCancelled) {
  std::vector<bms_parser::ChartMeta> chartMetas;
  dbHelper.SelectAllChartMeta(db, chartMetas);
  // sort by title
  std::sort(chartMetas.begin(), chartMetas.end(),
            [](bms_parser::ChartMeta &a, bms_parser::ChartMeta &b) {
              return a.Title < b.Title;
            });
  std::vector<Diff> diffs;
  std::cout << "Finding new bms files" << std::endl;
  auto entries = dbHelper.SelectAllEntries(db);
  std::unordered_set<path_t> oldFilesWs;

  for (auto &chartMeta : chartMetas) {
    if (isCancelled) {
      break;
    }
    oldFilesWs.insert(fspath_to_path_t(chartMeta.BmsPath));
    // std::cout << "Old file: " << chartMeta.BmsPath << std::endl;
    // std::cout << "Folder: " << chartMeta.Folder << std::endl;
  }
  for (auto &entry : entries) {
    if (isCancelled) {
      break;
    }
    FindNewBmsFiles(diffs, oldFilesWs, entry, isCancelled);
  }

  std::cout << "Found " << diffs.size() << " new bms files" << std::endl;
  std::atomic_bool is_committing(false);
  std::atomic_int success_count(0);
  dbHelper.BeginTransaction(db);
  parallel_for(diffs.size(), [&](int start, int end) {
    for (int i = start; i < end; i++) {
      if (isCancelled) {
        break;
      }
      auto &diff = diffs[i];
      if (diff.type == Added) {
        bms_parser::Parser parser;
        bms_parser::Chart *chart;
        std::atomic_bool cancel(false);
        bms_parser::ChartMeta chartMeta;
        try {
          parser.Parse(diffs[i].path, &chart, false, true, cancel);
        } catch (std::exception &e) {
          delete chart;
          std::cerr << "Error parsing " << diffs[i].path << ": " << e.what()
                    << std::endl;
          continue;
        }

        if (chart == nullptr)
          continue;
        ++success_count;
        if (success_count % 1000 == 0 && !is_committing) {
          is_committing = true;
          dbHelper.CommitTransaction(db);
          dbHelper.BeginTransaction(db);
          is_committing = false;
        }
        dbHelper.InsertChartMeta(db, chart->Meta);
        delete chart;
      }
    }
  });
  dbHelper.CommitTransaction(db);
  std::cout << "Inserted " << success_count << " new charts" << std::endl;
}

#ifdef _WIN32
void FindFilesWin(const std::filesystem::path &path, std::vector<Diff> &diffs,
                  const std::unordered_set<path_t> &oldFilesWs,
                  std::vector<path_t> &directoriesToVisit,
                  std::atomic_bool &isCancelled) {
  WIN32_FIND_DATAW findFileData;
  HANDLE hFind =
      FindFirstFileW((path.wstring() + L"\\*.*").c_str(), &findFileData);

  if (hFind != INVALID_HANDLE_VALUE) {
    do {
      if (isCancelled) {
        FindClose(hFind);
        break;
      }
      if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        path_t filename(findFileData.cFileName);

        if (filename.size() > 4) {
          path_t ext = filename.substr(filename.size() - 4);
          std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
          if (ext == L".bms" || ext == L".bme" || ext == L".bml") {
            path_t dirPath;

            path_t fullPath = path.wstring() + L"\\" + filename;
            if (oldFilesWs.find(fullPath) == oldFilesWs.end()) {
              diffs.push_back({fullPath, Added});
            }
          }
        }
      } else if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        path_t filename(findFileData.cFileName);

        if (filename != L"." && filename != L"..") {
          directoriesToVisit.push_back(path.wstring() + L"\\" + filename);
        }
      }
    } while (FindNextFileW(hFind, &findFileData) != 0);
    FindClose(hFind);
  }
}
#else
void resolveDType(const std::filesystem::path &directoryPath,
                  struct dirent *entry) {
  if (entry->d_type == DT_UNKNOWN) {
    std::filesystem::path fullPath = directoryPath / entry->d_name;
    struct stat statbuf;
    if (stat(fullPath.c_str(), &statbuf) == 0) {
      if (S_ISREG(statbuf.st_mode)) {
        entry->d_type = DT_REG;
      } else if (S_ISDIR(statbuf.st_mode)) {
        entry->d_type = DT_DIR;
      }
    }
  }
}
// TODO: Use platform-specific method for faster traversal
void FindFilesUnix(const std::filesystem::path &directoryPath,
                   std::vector<Diff> &diffs,
                   const std::unordered_set<path_t> &oldFiles,
                   std::vector<std::filesystem::path> &directoriesToVisit,
                   std::atomic_bool &isCancelled) {
  DIR *dir = opendir(directoryPath.c_str());
  if (dir) {
    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
      if (isCancelled) {
        closedir(dir);
        break;
      }
      resolveDType(directoryPath, entry);
      if (entry->d_type == DT_REG) {
        std::string filename = entry->d_name;
        if (filename.size() > 4) {
          std::string ext = filename.substr(filename.size() - 4);
          std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
          if (ext == ".bms" || ext == ".bme" || ext == ".bml") {
            std::filesystem::path fullPath = directoryPath / filename;
            if (oldFiles.find(fspath_to_path_t(fullPath)) == oldFiles.end()) {
              diffs.push_back({fullPath, Added});
            }
          }
        }
      } else if (entry->d_type == DT_DIR) {
        std::string filename = entry->d_name;
        if (filename != "." && filename != "..") {
          directoriesToVisit.push_back(directoryPath / filename);
        }
      }
    }
    closedir(dir);
  }
}

#endif

void FindNewBmsFiles(std::vector<Diff> &diffs,
                     const std::unordered_set<path_t> &oldFilesWs,
                     const std::filesystem::path &path,
                     std::atomic_bool &isCancelled) {
#ifdef _WIN32
  std::vector<path_t> directoriesToVisit;
  directoriesToVisit.push_back(path.wstring());
#else
  std::vector<std::filesystem::path> directoriesToVisit;
  directoriesToVisit.push_back(path);
#endif

  while (!directoriesToVisit.empty()) {
    if (isCancelled) {
      break;
    }
    std::filesystem::path currentDir = directoriesToVisit.back();
    directoriesToVisit.pop_back();

#ifdef _WIN32

    FindFilesWin(currentDir, diffs, oldFilesWs, directoriesToVisit,
                 isCancelled);
#else
    FindFilesUnix(currentDir, diffs, oldFilesWs, directoriesToVisit,
                  isCancelled);
#endif
  }
}
