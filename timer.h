#ifndef INC_TIMER_H_
#define INC_TIMER_H_
#include "yaka.h"
#include <iostream>
class Timer {
  uint64_t t;
public:
  Timer() : t(0) {}
  ~Timer() {}

  static uint64_t now();
  void start() { t = now(); }
  void stop() { t = now() - t; }
  uint64_t get_elapsed_ms() { return t ? t : 1; }

  friend std::ostream& operator<<(std::ostream& os, Timer& timer);
};

#endif
