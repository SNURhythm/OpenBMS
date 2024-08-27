//
// Created by XF on 8/28/2024.
//
#include "path.h"
std::string path_t_to_utf8(const path_t& input)
{
#ifdef _WIN32
  // Determine the size of the buffer needed
  int requiredSize = WideCharToMultiByte(65001 /* UTF8 */, 0, input.c_str(), -1, NULL, 0, NULL, NULL);

  if (requiredSize <= 0) {
    // Conversion failed, return an empty string
    return std::string();
  }

  // Create a string with the required size
  std::string result(requiredSize, '\0');

  // Perform the conversion
  WideCharToMultiByte(65001 /* UTF8 */, 0, input.c_str(), -1, &result[0], requiredSize, NULL, NULL);

  // Remove the extra null terminator added by WideCharToMultiByte
  result.resize(requiredSize - 1);

  return result;
#else
  // On non-Windows systems, just return the input string
  return path_t;
#endif
}