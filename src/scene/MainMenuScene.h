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
class MainMenuScene : public Scene {
public:
  inline explicit MainMenuScene() : Scene() {}
  void init(ApplicationContext &context) override;

  void update(float dt) override;
  void renderScene() override;
  void cleanupScene() override;

private:
  std::thread checkEntriesThread;
  RecyclerView<std::string> *recyclerView = nullptr;
  LinearLayout *rootLayout = nullptr;

  void initView();
  static void CheckEntries(ApplicationContext &context, MainMenuScene &scene);

  static void LoadCharts(ChartDBHelper &dbHelper, sqlite3 *db,
                         MainMenuScene &scene, std::atomic_bool &isCancelled);
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
#else
  static void
  FindFilesUnix(const std::filesystem::path &path, std::vector<Diff> &diffs,
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
