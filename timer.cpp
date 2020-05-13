#include "timer.h"
#include <chrono>

uint64_t Timer::now()
{
  using Clock = std::chrono::high_resolution_clock;
  static const double Ratio = (Clock::period().num * 1000.) / Clock::period().den;
  return uint64_t(Clock::now().time_since_epoch().count() * Ratio);
}

std::ostream& operator<<(std::ostream& os, Timer& timer)
{
  os << timer.get_elapsed_ms() << " ms";
  return os;
}