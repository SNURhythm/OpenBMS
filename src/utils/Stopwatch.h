//
// Created by XF on 9/2/2024.
//

#pragma once
#include <chrono>
class Stopwatch {
public:
  Stopwatch() : running(false), elapsed_time(0) {}

  void start();

  void pause();

  void resume();

  void reset();

  long long elapsedMicros() const;

  void seek(long long micro);

private:
  std::chrono::high_resolution_clock::time_point start_time;
  bool running;
  long long elapsed_time; // in milliseconds
};