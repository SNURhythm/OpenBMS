#include <sndfile.h> // Include the sndfile library for handling audio files

#include <iostream>
#include "decoder.h"

// Function to decode audio file to PCM
std::vector<short> decodeAudioToPCM(const char *filePath) {
  // Open the audio file
  SF_INFO fileInfo;

  SNDFILE *file = sf_open(filePath, SFM_READ, &fileInfo);

  if (!file) {
    std::cerr << "Error opening file: " << filePath << std::endl;
    return {};
  }

  // Prepare a buffer to hold the PCM data
  std::vector<short> buffer(fileInfo.frames * fileInfo.channels);

  // Read the audio data into the buffer
  sf_count_t numFrames = sf_readf_short(file, buffer.data(), fileInfo.frames);

  if (numFrames < 1) {
    std::cerr << "Failed to read audio data." << std::endl;
    sf_close(file);
    return {};
  }

  // Close the file
  sf_close(file);

  // Return the PCM data
  return buffer;
}