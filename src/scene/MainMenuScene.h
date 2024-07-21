#pragma once
#include "../view/RecyclerView.h"
#include "../view/LinearLayout.h"
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
class MainMenuScene : public Scene {
public:
  inline explicit MainMenuScene() : Scene() {}
  void init(ApplicationContext &context) override;

  void update(float dt) override;
  void renderScene() override;
  void cleanupScene() override;

private:
  std::atomic_bool previewLoadCancelled = false;
  Jukebox jukebox;
  VideoPlayer videoPlayer;
  std::thread previewThread;
  std::thread checkEntriesThread;
  RecyclerView<bms_parser::ChartMeta> *recyclerView = nullptr;
  LinearLayout *rootLayout = nullptr;

  void initView(ApplicationContext &context);
  static void CheckEntries(ApplicationContext &context, MainMenuScene &scene);

  static void LoadCharts(ChartDBHelper &dbHelper, sqlite3 *db,
                         std::vector<path_t> &entries, MainMenuScene &scene,
                         std::atomic_bool &isCancelled);
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
                           std::atomic_bool &isCancelled);
#elif TARGET_OS_OSX || TARGET_OS_LINUX
  static void
  FindFilesUnix(const std::filesystem::path &path, std::vector<Diff> &diffs,
                const std::unordered_set<path_t> &oldFilesWs,
                std::vector<std::filesystem::path> &directoriesToVisit,
                std::atomic_bool &isCancelled);
#elif TARGET_OS_IPHONE
  static void
  FindFilesIOS(const std::filesystem::path &path, std::vector<Diff> &diffs,
               const std::unordered_set<path_t> &oldFilesWs,
               std::vector<std::filesystem::path> &directoriesToVisit,
               std::atomic_bool &isCancelled);
#endif
  static void FindNewBmsFiles(std::vector<Diff> &diffs,
                              const std::unordered_set<path_t> &oldFilesWs,
                              const std::filesystem::path &path,
                              std::atomic_bool &isCancelled);
  static void resolveDType(const std::filesystem::path &directoryPath,
                           struct dirent *entry);
};
