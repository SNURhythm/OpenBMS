#pragma once

class IRhythmControl {
public:
  virtual ~IRhythmControl() = default;
  virtual int pressLane(int mainLane, int compensateLane,
                        double inputDelay = 0) = 0;
  virtual int pressLane(int lane, double inputDelay = 0) = 0;
  virtual void releaseLane(int lane, double inputDelay = 0) = 0;
};
