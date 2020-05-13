#include "bitboard.h"
#include <sstream>

namespace Bitboard {
  uint64_t SquareMask[Square::SQ_NB]; // 1ULL << sq
  uint64_t ThisAndNextSq[Square::SQ_NB];  // 3ULL << sq
  uint64_t PrevSquares[Square::SQ_NB];
#ifndef USE_INTRIN
  Square::Type BitScanTable[64];
#endif

  void init()
  {
    for (Square::Type sq = Square::A1; sq < Square::SQ_NB; ++sq) {
      SquareMask[sq] = 1ULL << sq;
      ThisAndNextSq[sq] = 3ULL << sq;
      PrevSquares[sq] = (sq_mask(sq) - 1) + (sq == 0);
#ifndef USE_INTRIN
      BitScanTable[((SquareMask[sq] | PrevSquares[sq]) * DeBruijn64) >> 58] = sq;
#endif
    }
  }

  std::string to_str(uint64_t b)
  {
    std::ostringstream ss;
    for (Square::Rank r = Square::RANK_8; r >= Square::RANK_1; --r) {
      ss << "    +---+---+---+---+---+---+---+---+\n";
      ss.width(3);
      ss << r + 1 << " |";
      for (Square::File f = Square::FILE_A; f <= Square::FILE_H; ++f)
        ss << (bit_set(b, Square::square_of(f, r)) ? " X |" : "   |");
      ss << '\n';
    }
    ss << "    +---+---+---+---+---+---+---+---+\n";
    ss << "      a   b   c   d   e   f   g   h  \n";
    return ss.str();
  }
}