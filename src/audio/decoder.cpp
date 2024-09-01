#include <iostream>
#include "decoder.h"
#include <SDL2/SDL.h>
#include <algorithm>
#ifdef _WIN32
#define sf_open sf_wchar_open
#endif
// Function to decode audio file to PCM
bool decodeAudioToPCM(const path_t &filePath, std::vector<short> &buffer,
                      SF_INFO &fileInfo) {
  // Open the audio file
  SNDFILE *file = sf_open(filePath.c_str(), SFM_READ, &fileInfo);

  if (!file) {
    SDL_Log("Failed to open audio file %ls, error: %s", filePath.c_str(),
            sf_strerror(file));
    return false;
  }

  // Prepare a buffer to hold the PCM data
  buffer.resize(fileInfo.frames * fileInfo.channels, 0);
  // Read the audio data into the buffer
  std::vector<double> tempBuffer(fileInfo.frames * fileInfo.channels);
  sf_count_t numFrames =
      sf_readf_double(file, tempBuffer.data(), fileInfo.frames);
  if (numFrames < 0) {
    SDL_Log("Failed to read audio data from file %s, error: %s",
            filePath.c_str(), sf_strerror(file));
    return false;
  }
  // Convert the double buffer to short
  std::transform(
      tempBuffer.begin(), tempBuffer.end(), buffer.begin(), [](double val) {
        return static_cast<short>(std::clamp(val, -1.0, 1.0) * 32767);
      });

  if (numFrames < fileInfo.frames) {
    SDL_Log("Failed to read all audio data from file %s, read %lld frames",
            filePath.c_str(), numFrames);
    // Zero out the remaining buffer
    std::fill(buffer.begin() + numFrames * fileInfo.channels, buffer.end(), 0);
  }

  // Close the file
  sf_close(file);
  return true;
}