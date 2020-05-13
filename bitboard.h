#ifndef INC_BITBOARD_H_
#define INC_BITBOARD_H_
#include "yaka.h"
#ifdef USE_INTRIN
#include <intrin.h>
#endif

namespace Bitboard {
  const uint64_t Universe = 0xFFFFFFFFFFFFFFFFULL;
  const uint64_t LightSquares = ~0xAA55AA55AA55AA55ULL;
  const uint64_t DarkSquares = 0xAA55AA55AA55AA55ULL;
  extern uint64_t SquareMask[Square::SQ_NB];
  extern uint64_t ThisAndNextSq[Square::SQ_NB];
  extern uint64_t PrevSquares[Square::SQ_NB];
  // File Masks
  const uint64_t FileAMask = 0x0101010101010101ULL;
  const uint64_t FileBMask = 0x0202020202020202ULL;
  const uint64_t FileCMask = 0x0404040404040404ULL;
  const uint64_t FileDMask = 0x0808080808080808ULL;
  const uint64_t FileEMask = 0x1010101010101010ULL;
  const uint64_t FileFMask = 0x2020202020202020ULL;
  const uint64_t FileGMask = 0x4040404040404040ULL;
  const uint64_t FileHMask = 0x8080808080808080ULL;
  const uint64_t FileMask[Square::FILE_NB] = {
    FileAMask, FileBMask, FileCMask, FileDMask,
    FileEMask, FileFMask, FileGMask, FileHMask
  };

  // Rank Masks
  const uint64_t Rank1Mask = 0x00000000000000FFULL;
  const uint64_t Rank2Mask = 0x000000000000FF00ULL;
  const uint64_t Rank3Mask = 0x0000000000FF0000ULL;
  const uint64_t Rank4Mask = 0x00000000FF000000ULL;
  const uint64_t Rank5Mask = 0x000000FF00000000ULL;
  const uint64_t Rank6Mask = 0x0000FF0000000000ULL;
  const uint64_t Rank7Mask = 0x00FF000000000000ULL;
  const uint64_t Rank8Mask = 0xFF00000000000000ULL;
  const uint64_t RankMask[Square::RANK_NB] = {
    Rank1Mask, Rank2Mask, Rank3Mask, Rank4Mask,
    Rank5Mask, Rank6Mask, Rank7Mask, Rank8Mask
  };

  // Diagonal Masks
  const uint64_t DiagMask[Square::DIAG_NB] = {
    0x8040201008040201ULL, 0x4020100804020100ULL, 0x2010080402010000ULL,
    0x1008040201000000ULL, 0x0804020100000000ULL, 0x0402010000000000ULL,
    0x0201000000000000ULL, 0x0100000000000000ULL,
    0x0000000000000000ULL,
    0x0000000000000080ULL, 0x0000000000008040ULL, 0x0000000000804020ULL,
    0x0000000080402010ULL, 0x0000008040201008ULL, 0x0000804020100804ULL,
    0x0080402010080402ULL
  };

  // Anti Diagonal Masks
  const uint64_t ADiagMask[Square::DIAG_NB] = {
    0x0102040810204080ULL, 0x0001020408102040ULL, 0x0000010204081020ULL,
    0x0000000102040810ULL, 0x0000000001020408ULL, 0x0000000000010204ULL,
    0x0000000000000102ULL, 0x0000000000000001ULL,
    0x0000000000000000ULL,
    0x8000000000000000ULL, 0x4080000000000000ULL, 0x2040800000000000ULL,
    0x1020408000000000ULL, 0x0810204080000000ULL, 0x0408102040800000ULL,
    0x0204081020408000ULL
  };

  inline uint64_t sq_mask(Square::Type sq)
  {
    return SquareMask[sq];
  }
  template <typename Sq, typename... Rest>
  inline uint64_t sq_mask(Sq sq, Rest... rest)
  {
    return sq_mask(sq) | sq_mask(rest...);
  }

#ifndef USE_INTRIN
  const uint64_t DeBruijn64 = 0x03F79D71B4CB0A89ULL;
  extern Square::Type BitScanTable[64];
#endif

  inline Square::Type lsb(uint64_t b)
  {
#ifdef USE_INTRIN
    unsigned long idx;
    _BitScanForward64(&idx, b);
    return (Square::Type) idx;
#else
    return BitScanTable[((b ^ (b - 1)) * DeBruijn64) >> 58];
#endif
  }

  inline Square::Type msb(uint64_t b)
  {
    if (!b) {
      exit(EXIT_FAILURE);
    }
#ifdef USE_INTRIN
    unsigned long idx;
    _BitScanReverse64(&idx, b);
    return (Square::Type) idx;
#else
    b |= (b |= (b |= (b |= (b |= b >> 1) >> 2) >> 4) >> 8) >> 16;
    return BitScanTable[((b | b >> 32) * DeBruijn64) >> 58];
#endif
  }

  inline count_t bit_count(uint64_t b)
  {
#ifdef USE_INTRIN
    return (count_t)__popcnt64(b);
#else
    b -= (b >> 1) & 0x5555555555555555ULL;
    b = ((b >> 2) & 0x3333333333333333ULL) + (b & 0x3333333333333333ULL);
    return ((((b >> 4) + b) & 0x0F0F0F0F0F0F0F0FULL) * 0x0101010101010101ULL) >> 56;
#endif
  }

  inline count_t sparse_bit_count(uint64_t b)
  {
#ifdef USE_INTRIN
    return bit_count(b);
#else
#define HANDLE(x) if (b == 0) return x; b &= b - 1;
#define HANDLE4(x) HANDLE(x); HANDLE(x+1); HANDLE(x+2); HANDLE(x+3)
#define HANDLE16(x) HANDLE4(x); HANDLE4(x+4); HANDLE4(x+8); HANDLE4(x+12)
    HANDLE16(0);
    HANDLE16(16);
    HANDLE16(16+16);
    HANDLE16(16+16+16);
    return 64;
#undef HANDLE
#endif
  }

  inline count_t bit_count8(uint8_t b)
  {
    static const count_t BitCountTable[] = {
#define B2(n)    n,     n + 1,     n + 1,     n + 2
#define B4(n) B2(n), B2(n + 1), B2(n + 1), B2(n + 2)
#define B6(n) B4(n), B4(n + 1), B4(n + 1), B4(n + 2)
      B6(0), B6(0 + 1), B6(0 + 1), B6(0 + 2)
#undef B2
#undef B4
#undef B6
    };
    return BitCountTable[b];
  }
  inline uint64_t byteswap(uint64_t b)
  {
#ifdef USE_INTRIN
    return _byteswap_uint64(b);
#else
    b = ((b >> 8) & 0x00FF00FF00FF00FFULL) | ((b & 0x00FF00FF00FF00FFULL) << 8);
    b = ((b >> 16) & 0x0000FFFF0000FFFFULL) | ((b & 0x0000FFFF0000FFFFULL) << 16);
    return (b >> 32) | (b << 32);
#endif
  }

  inline Square::Type pop_1st_bit(uint64_t * const b)
  {
    Square::Type s = lsb(*b); // Store lsb
    (*b) &= (*b) - 1; // Clear lsb
    return s; // Return lsb
  }

  template <int Dir>
  inline uint64_t shift_bb(uint64_t b)
  {
    static_assert((Dir == 8) || (Dir == -8) || (Dir == 9) || (Dir == -9)
                  || (Dir == 7) || (Dir == -7) || (Dir == 1) || (Dir == -1)
                  || (Dir == 16) || (Dir == -16)
                  , "Invalid Direction");
    return Dir == 8 ? (b << 8) : Dir == -8 ? (b >> 8)
      : Dir == 9 ? (b << 9) &~FileAMask : Dir == -9 ? (b >> 9) &~FileHMask
      : Dir == 7 ? (b << 7) &~FileHMask : Dir == -7 ? (b >> 7) &~FileAMask
      : Dir == 1 ? (b << 1) &~FileAMask : Dir == -1 ? (b >> 1) &~FileHMask
      : Dir == 16 ? b << 16 : Dir == -16 ? b >> 16 : 0;
  }

  inline uint64_t north_fill(uint64_t b)
  {
    b |= b << 8;
    b |= b << 16;
    return b | (b << 32);
  }

  inline uint64_t south_fill(uint64_t b)
  {
    b |= b >> 8;
    b |= b >> 16;
    return b | (b >> 32);
  }

  inline uint64_t full_fill(uint64_t b)
  {
    b |= (b << 8) | (b >> 8);
    b |= (b << 16) | (b >> 16);
    return (b << 32) | (b >> 32);
  }

  template <Color::Type C>
  inline uint64_t forward_fill(uint64_t b)
  {
    return C == Color::WHITE ? north_fill(b) : south_fill(b);
  }

  template <Color::Type C>
  inline uint64_t backward_fill(uint64_t b)
  {
    return forward_fill<Color::Type(!C)>(b);
  }

  inline bool more_than_one(uint64_t b) { return (b & (b - 1)) != 0; }
  inline bool bit_set(uint64_t b, Square::Type sq) { return (b & sq_mask(sq)) != 0; }
  void init();
  std::string to_str(uint64_t b);
}

#endif
