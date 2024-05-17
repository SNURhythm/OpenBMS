#pragma once
#include "targets.h"
#if TARGET_OS_IOS
#include <string>
// get Documents path
std::string GetIOSDocumentsPath();
void RegisterTouchEvent();
#endif
