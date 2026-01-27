#pragma once
#include "targets.h"
#if TARGET_OS_IOS || TARGET_OS_SIMULATOR
#include <string>
#include <vector>
// get Documents path
std::string GetIOSDocumentsPath();
void *GetIOSWindowHandle(void *uiwindow);
void RegisterTouchEvent();
std::vector<std::string> ListDocumentFilesRecursively();
#endif
