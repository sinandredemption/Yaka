#ifndef INC_RANDOM_H_
#define INC_RANDOM_H_

#include "yaka.h"
namespace Random {
  extern uint64_t s;
  uint64_t rand64();
  uint64_t rand64_few_bits();
  size_t rand_int(size_t max); // In the interval [0, max)
};

#endif
