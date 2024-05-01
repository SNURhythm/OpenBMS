#pragma once
#if PLATFORM_MAC
#include <string>
#import <Cocoa/Cocoa.h>
#include "Mac/CocoaThread.h"
bool MacOSSelectFolder(const char* title, const char* defaultPath, std::string* OutFolderName);

#endif
