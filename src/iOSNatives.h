#pragma once
#if PLATFORM_IOS
#include <filesystem>
// get Documents path
std::filesystem::path GetIOSDocumentsPath();
#endif
