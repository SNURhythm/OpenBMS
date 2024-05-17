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
#elif __linux
#elif __unix // all unices not caught above
// Unix
#elif __posix
// POSIX
#endif