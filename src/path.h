#pragma once
#include <filesystem>
#include <string>
#ifdef _WIN32
typedef std::wstring path_t;
#else
typedef std::string path_t;
#endif

path_t wstring_to_path_t(const std::wstring &wstr);

path_t fspath_to_path_t(const std::filesystem::path &fspath);