#include "path.h"
#include "Utils.h"

path_t wstring_to_path_t(const std::wstring &wstr) {
#ifdef _WIN32
  return std::wstring(wstr);
#else
  return ws2s(wstr);
#endif
}

path_t fspath_to_path_t(const std::filesystem::path &fspath) {
#ifdef _WIN32
  return fspath.wstring();
#else
  return fspath.string();
#endif
}
