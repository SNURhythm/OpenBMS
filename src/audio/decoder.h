#pragma once
#include <sndfile.h> // Include the sndfile library for handling audio files
#include <vector>
#include "../path.h"
std::vector<short> decodeAudioToPCM(const path_t &filePath,
                                    std::vector<short> &buffer,
                                    SF_INFO &fileInfo);