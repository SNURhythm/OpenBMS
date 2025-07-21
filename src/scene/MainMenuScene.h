#pragma once
#include "../view/RecyclerView.h"
#include "Scene.h"
#include "../ChartDBHelper.h"
#include "../path.h"
#include <filesystem>
#include <thread>
#include <unordered_set>
#include <vector>
#include "../targets.h"
#include "../audio/Jukebox.h"
#include "../video/VideoPlayer.h"
#include <atomic>
#include <stop_token>

class MainMenuScene : public Scene {
public:
  inline explicit MainMenuScene(ApplicationContext &context) : Scene(context) {}
  void init() override;

  void update(float dt) override;
  void renderScene() override;
  void cleanupScene() override;

private:
  sqlite3 *db;
  std::atomic_bool previewLoadCancelled = false;
  bool willStart = false;
  bms_parser::Chart *selectedChart = nullptr;

  std::thread loadThread;
  std::jthread checkEntriesThread;
  RecyclerView<bms_parser::ChartMeta> *recyclerView = nullptr;
  View *rootLayout = nullptr;

  void initView(ApplicationContext &context);
  static void CheckEntries(std::stop_token stop_token,
                           ApplicationContext &context, MainMenuScene &scene);

  static void LoadCharts(ChartDBHelper &dbHelper, sqlite3 *db,
                         std::vector<path_t> &entries, MainMenuScene &scene,
                         std::stop_token stop_token);
  enum DiffType { Deleted, Added };
  struct Diff {
    std::filesystem::path path;
    DiffType type;
  };
#ifdef _WIN32
  static void FindFilesWin(const std::filesystem::path &path,
                           std::vector<Diff> &diffs,
                           const std::unordered_set<path_t> &oldFilesWs,
                           std::vector<path_t> &directoriesToVisit,
                           std::stop_token stop_token);
#elif TARGET_OS_OSX || TARGET_OS_LINUX
  static void
  FindFilesUnix(const std::filesystem::path &path, std::vector<Diff> &diffs,
                const std::unordered_set<path_t> &oldFilesWs,
                std::vector<std::filesystem::path> &directoriesToVisit,
                std::stop_token stop_token);
#elif TARGET_OS_IPHONE
  static void
  FindFilesIOS(const std::filesystem::path &path, std::vector<Diff> &diffs,
               const std::unordered_set<path_t> &oldFilesWs,
               std::vector<std::filesystem::path> &directoriesToVisit,
               std::stop_token stop_token);
#endif
  static void FindNewBmsFiles(std::vector<Diff> &diffs,
                              const std::unordered_set<path_t> &oldFilesWs,
                              const std::filesystem::path &path,
                              std::stop_token stop_token);
  static void resolveDType(const std::filesystem::path &directoryPath,
                           struct dirent *entry);
};
