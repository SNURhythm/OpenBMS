

#include <iostream>
#include "decoder.h"
#include <SDL2/SDL.h>
#ifdef _WIN32
#define sf_open sf_wchar_open
#endif
// Function to decode audio file to PCM
std::vector<short> decodeAudioToPCM(const path_t& filePath, SF_INFO& fileInfo) {
  // Open the audio file

  SNDFILE *file = sf_open(filePath.c_str(), SFM_READ, &fileInfo);

  if (!file) {
    SDL_Log("Failed to open audio file %ls, error: %s", filePath.c_str(), sf_strerror(file));
    return {};
  }

  // Prepare a buffer to hold the PCM data
  std::vector<short> buffer(fileInfo.frames * fileInfo.channels);

  // Read the audio data into the buffer
  sf_count_t numFrames = sf_readf_short(file, buffer.data(), fileInfo.frames);

  if (numFrames < 1) {
    SDL_Log("Failed to read audio data from file %ls", filePath.c_str());
    sf_close(file);
    return {};
  }

  // Close the file
  sf_close(file);

  // Return the PCM data
  return buffer;
}