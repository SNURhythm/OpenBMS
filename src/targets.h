#pragma once
#ifdef _WIN32
#include <windows.h>

#elif __APPLE__

#include "TargetConditionals.h"
#if TARGET_OS_IPHONE && TARGET_IPHONE_SIMULATOR
// define something for simulator
#elif TARGET_OS_IPHONE
#else
#define TARGET_OS_OSX 1
#endif
#elif __linux__
#define TARGET_OS_LINUX 1
#elif __unix // all unices not caught above
// Unix
#elif __posix
// POSIX
#endif

enum TargetPlatform {
  Windows,
  MacOS,
  Linux,
  iOS
};

#if TARGET_OS_OSX
constexpr TargetPlatform TARGET_PLATFORM = MacOS;
#elif TARGET_OS_IOS
constexpr TargetPlatform TARGET_PLATFORM = iOS;
#elif TARGET_OS_IPHONE
constexpr TargetPlatform TARGET_PLATFORM = iOS;
#elif TARGET_OS_LINUX
constexpr TargetPlatform TARGET_PLATFORM = Linux;
#else
constexpr TargetPlatform TARGET_PLATFORM = Windows;
#endif