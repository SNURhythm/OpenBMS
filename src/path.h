#pragma once
#include <filesystem>
#include <string>
#ifdef _WIN32
#include <windows.h>
typedef std::wstring path_t;
#else
typedef std::string path_t;
#endif

#ifdef _WIN32
#define wstring_to_path_t(wstr) (wstr)
#define fspath_to_path_t(fspath) (fspath.wstring())
#else
#define wstring_to_path_t(wstr) (std::string().assign(wstr.begin(), wstr.end()))
#define fspath_to_path_t(fspath) (fspath.string())
#endif

std::string path_t_to_utf8(const path_t &input);
