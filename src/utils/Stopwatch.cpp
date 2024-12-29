//
// Created by XF on 9/2/2024.
//

#include "Stopwatch.h"
void Stopwatch::start() {
  if (!running) {
    start_time = std::chrono::high_resolution_clock::now();
    running = true;
  }
}
void Stopwatch::pause() {
  if (running) {
    auto pause_time = std::chrono::high_resolution_clock::now();
    elapsed_time += std::chrono::duration_cast<std::chrono::microseconds>(
                        pause_time - start_time)
                        .count();
    running = false;
  }
}
void Stopwatch::resume() {
  if (!running) {
    start_time = std::chrono::high_resolution_clock::now();
    running = true;
  }
}
void Stopwatch::reset() {
  running = false;
  elapsed_time = 0;
}
long long Stopwatch::elapsedMicros() const {
  if (running) {
    auto current_time = std::chrono::high_resolution_clock::now();
    return elapsed_time + std::chrono::duration_cast<std::chrono::microseconds>(
                              current_time - start_time)
                              .count();
  }
  return elapsed_time;
}
void Stopwatch::seek(long long int micro) {
  elapsed_time = micro;
  start_time = std::chrono::high_resolution_clock::now();
}
bool Stopwatch::isRunning() const { return running; }
