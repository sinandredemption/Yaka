#include "random.h"
#include <limits>

namespace Random {
  uint64_t s = 3141592653589793238ULL;
  uint64_t rand64()
  {
    /*s ^= s >> 11;
    s ^= s << 7;
    s ^= s << 15;
    s ^= s >> 18;*/
    s ^= (s ^= (s ^= s >> 12) << 25) >> 27; /* xorshift64star */
    return s * 0x2545F4914F6CDD1DULL;
  }

  uint64_t rand64_few_bits()
  {
    return rand64() & rand64() & rand64();
  }

  size_t rand_int(size_t max) // In the interval [0, max)
  {
    static const double Max = double(std::numeric_limits<uint64_t>::max());
    size_t x = size_t((rand64() / Max) * max);
    if (x == max) return max - 1;
    else return x;
  }
};