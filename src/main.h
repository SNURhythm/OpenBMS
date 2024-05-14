#include "ChartDBHelper.h"
#include "Utils.h"
#include "bms_parser.hpp"
#include "path.h"
#include "sqlite3.h"
#include "tinyfiledialogs.h"
#include "vlcpp/vlc.hpp"
#include <filesystem>
#include <string>
#include <unordered_set>
#include <vector>
int main(int argv, char **args);
void LoadCharts(ChartDBHelper &dbHelper, sqlite3 *db,
                std::atomic_bool &isCancelled);
enum DiffType { Deleted, Added };
struct Diff {
  std::filesystem::path path;
  DiffType type;
};
#ifdef _WIN32
void FindFilesWin(const std::filesystem::path &path, std::vector<Diff> &diffs,
                  const std::unordered_set<path_t> &oldFilesWs,
                  std::vector<path_t> &directoriesToVisit,
                  std::atomic_bool &isCancelled);
#else
void FindFilesUnix(const std::filesystem::path &path, std::vector<Diff> &diffs,
                   const std::unordered_set<path_t> &oldFilesWs,
                   std::vector<std::filesystem::path> &directoriesToVisit,
                   std::atomic_bool &isCancelled);
#endif
void FindNewBmsFiles(std::vector<Diff> &diffs,
                     const std::unordered_set<path_t> &oldFilesWs,
                     const std::filesystem::path &path,
                     std::atomic_bool &isCancelled);
