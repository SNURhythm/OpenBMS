#include "ChartDBHelper.h"
#include "sqlite3.h"
#include <vector>
#include <unordered_set>
#include <string>
#include <filesystem>
#include "Utils.h"

#include "vlcpp/vlc.hpp"
#include "bms_parser.hpp"
#include "tinyfiledialogs.h"
int main();
void LoadCharts(ChartDBHelper& dbHelper, sqlite3* db, std::atomic_bool& isCancelled);
enum DiffType
{
  Deleted,
  Added
};
struct Diff
{
  std::filesystem::path path;
  DiffType type;
};
#ifdef _WIN32
void FindFilesWin(const std::filesystem::path& path, std::vector<Diff>& diffs, const std::unordered_set<std::wstring>& oldFilesWs, std::vector<std::wstring>& directoriesToVisit, std::atomic_bool& isCancelled);
#else
void FindFilesUnix(const std::filesystem::path& path, std::vector<Diff>& diffs, const std::unordered_set<std::wstring>& oldFilesWs, std::vector<std::filesystem::path>& directoriesToVisit, std::atomic_bool& isCancelled);
#endif
void FindNewBmsFiles(std::vector<Diff>& diffs, const std::unordered_set<std::wstring>& oldFilesWs, const std::filesystem::path& path, std::atomic_bool& isCancelled);

