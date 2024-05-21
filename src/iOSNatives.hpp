#pragma once
#include "targets.h"
#if TARGET_OS_IOS
#include <string>
// get Documents path
std::string GetIOSDocumentsPath();
void *GetIOSWindowHandle(void *uiwindow);
void RegisterTouchEvent();
std::vector<std::string> ListDocumentFilesRecursively();
#endif
