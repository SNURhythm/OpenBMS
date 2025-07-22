#ifdef _WIN32
#include <windows.h>
#define TARGET_OS_WINDOWS 1
constexpr bool TARGET_IS_WINDOWS = true;
constexpr bool TARGET_IS_APPLE = false;
constexpr bool TARGET_IS_LINUX = false;
constexpr bool TARGET_IS_OSX = false;
constexpr bool TARGET_IS_IOS = false;
constexpr bool TARGET_IS_IOS_SIMULATOR = false;

#elif __APPLE__

#include "TargetConditionals.h"
#if TARGET_OS_IPHONE && TARGET_IPHONE_SIMULATOR
// define something for simulator
#define TARGET_OS_IOS_SIMULATOR 1
constexpr bool TARGET_IS_WINDOWS = false;
constexpr bool TARGET_IS_APPLE = true;
constexpr bool TARGET_IS_LINUX = false;
constexpr bool TARGET_IS_OSX = false;
constexpr bool TARGET_IS_IOS = true;
constexpr bool TARGET_IS_IOS_SIMULATOR = true;
#elif TARGET_OS_IPHONE
#define TARGET_OS_IOS 1
constexpr bool TARGET_IS_WINDOWS = false;
constexpr bool TARGET_IS_APPLE = true;
constexpr bool TARGET_IS_LINUX = false;
constexpr bool TARGET_IS_OSX = false;
constexpr bool TARGET_IS_IOS = true;
constexpr bool TARGET_IS_IOS_SIMULATOR = true;
#else
#define TARGET_OS_OSX 1
constexpr bool TARGET_IS_WINDOWS = false;
constexpr bool TARGET_IS_APPLE = true;
constexpr bool TARGET_IS_LINUX = false;
constexpr bool TARGET_IS_OSX = true;
constexpr bool TARGET_IS_IOS = false;
constexpr bool TARGET_IS_IOS_SIMULATOR = false;
#endif
#elif __linux
#define TARGET_OS_LINUX 1
constexpr bool TARGET_IS_WINDOWS = false;
constexpr bool TARGET_IS_APPLE = false;
constexpr bool TARGET_IS_LINUX = true;
constexpr bool TARGET_IS_OSX = false;
constexpr bool TARGET_IS_IOS = false;
constexpr bool TARGET_IS_IOS_SIMULATOR = false;
#elif __unix // all unices not caught above
// Unix
constexpr bool TARGET_IS_WINDOWS = false;
constexpr bool TARGET_IS_APPLE = false;
constexpr bool TARGET_IS_LINUX = false;
constexpr bool TARGET_IS_OSX = false;
constexpr bool TARGET_IS_IOS = false;
constexpr bool TARGET_IS_IOS_SIMULATOR = false;
#elif __posix
// POSIX
constexpr bool TARGET_IS_WINDOWS = false;
constexpr bool TARGET_IS_APPLE = false;
constexpr bool TARGET_IS_LINUX = false;
constexpr bool TARGET_IS_OSX = false;
constexpr bool TARGET_IS_IOS = false;
constexpr bool TARGET_IS_IOS_SIMULATOR = false;
#endif