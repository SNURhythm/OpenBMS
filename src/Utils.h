#pragma once
#ifdef _WIN32
#include <Windows.h>
#include <ShlObj.h>
#endif
#if PLATFORM_IOS
#include "iOSNatives.h"
#endif
#if PLATFORM_ANDROID
#include "AndroidNatives.h"
#endif
#include <string>
#include <filesystem>
#include <algorithm>
#include <functional>
#include <thread>
#include <vector>

void parallel_for(size_t n, std::function<void(int start, int end)> f);

std::string ws2s(const std::wstring &wstr);

std::string ws2s_utf8(const std::wstring &wstr);

class Utils
{
public:
    inline static std::string GameName = "OpenBMS";
    inline static std::string TeamName = "SNURhythm";
    static std::filesystem::path GetDocumentsPath(const std::filesystem::path& SubPath = "");
};

class threadRAII
{
    std::thread th;

public:
    explicit threadRAII(std::thread &&_th);

    ~threadRAII();
};