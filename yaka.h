#ifndef INC_YAKA_H_
#define INC_YAKA_H_

#include <cassert>
#include <string>
#include <cstdint>

typedef int32_t count_t;
typedef uint32_t depth_t;
typedef uint32_t hash_t;
typedef uint64_t key_t;

namespace Program {
  extern std::string StartFen;
  extern std::string Name;
  extern std::string HardwareInfo;
  extern std::string Version;
#ifndef NDEBUG
  const bool Debugging = true;
#else
  const bool Debugging = false;
#endif
  extern std::string Authors;
  std::string info();
  std::string uci_info();
}

namespace Color {
  enum Type {
    WHITE, BLACK,
    NONE,
    COLOR_NB = 2
  };

  inline Type operator~(const Type& c)
  {
    return Type(int(c) ^ 1);
  }

  char to_char(Type c);
  std::string to_str(Type c);
}

namespace Square {
  enum File {
    FILE_A = 0, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, FILE_NB = 8
  };

  enum Rank {
    RANK_1 = 0, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_NB = 8
  };

  enum Type {
    A1 = 0, B1 = 1, C1 = 2, D1 = 3, E1 = 4, F1 = 5, G1 = 6, H1 = 7, A2 =
    8, B2 = 9, C2 = 10, D2 = 11, E2 = 12, F2 = 13, G2 = 14, H2 = 15, A3 =
    16, B3 = 17, C3 = 18, D3 = 19, E3 = 20, F3 = 21, G3 = 22, H3 = 23, A4 =
    24, B4 = 25, C4 = 26, D4 = 27, E4 = 28, F4 = 29, G4 = 30, H4 = 31, A5 =
    32, B5 = 33, C5 = 34, D5 = 35, E5 = 36, F5 = 37, G5 = 38, H5 = 39, A6 =
    40, B6 = 41, C6 = 42, D6 = 43, E6 = 44, F6 = 45, G6 = 46, H6 = 47, A7 =
    48, B7 = 49, C7 = 50, D7 = 51, E7 = 52, F7 = 53, G7 = 54, H7 = 55, A8 =
    56, B8 = 57, C8 = 58, D8 = 59, E8 = 60, F8 = 61, G8 = 62, H8 = 63,
    SQ_NB, NONE = -1, DIAG_NB = 16, ADIAG_NB = 16
  };

  inline Type square_of(File fl, Rank rk)
  {
    assert(fl < FILE_NB);
    assert(rk < RANK_NB);
    return Type((rk << 3) | fl);
  }

  inline Type square_of(const char str[2])
  {
    return square_of(File(str[0] - 'a'), Rank(str[1] - '1'));
  }

  inline Type operator~(const Type& sq)
  {
    assert(sq < SQ_NB);
    return Type(sq ^ 56);
  }

  template <Color::Type Really = Color::BLACK>
  inline Type flip(Square::Type sq)
  {
    return Type(int(sq) ^ (56 * Really));
  }

  inline File file_of(Type sq)
  {
    assert(sq < SQ_NB);
    return File(sq & 7); // sq % 8
  }

  inline Rank rank_of(Type sq)
  {
    assert(sq < SQ_NB);
    return Rank(sq >> 3);  // sq / 8
  }

  inline int diag_of(Square::Type sq)
  {
    return (rank_of(sq) - file_of(sq)) & 15;
  }
  inline int adiag_of(Square::Type sq)
  {
    return (rank_of(sq) + file_of(sq)) ^ 7;
  }

  inline Type south(Square::Type sq, Color::Type cl)
  {
    return Type(sq + (cl << 4) - 8);
  }

  std::string to_str(Type sq);
}

namespace Piece {
  extern std::string PieceToChar;
  enum Type {
    WHITE_PAWN, BLACK_PAWN,
    WHITE_KNIGHT, BLACK_KNIGHT,
    WHITE_BISHOP, BLACK_BISHOP,
    WHITE_ROOK, BLACK_ROOK,
    WHITE_QUEEN, BLACK_QUEEN,
    WHITE_KING, BLACK_KING,
    NONE,
    ALL_PIECES = 12,
    PIECE_NB = 13
  };
  inline Type operator~(const Type& pc)
  {
    assert(pc < PIECE_NB);
    return Type(pc ^ 1);
  }

  inline Color::Type color_of(Type pc)
  {
    assert(pc < PIECE_NB);
    return Color::Type(pc & 1);
  }

  enum PieceType {
    PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, NO_PIECE_TYPE,
    ALL_PIECE_TYPES = 6, PIECE_TYPE_NB = 6
  };

  extern std::string PieceTypeToChar;

  inline Type make_piece(PieceType pt, Color::Type c)
  {
    assert(pt < PIECE_TYPE_NB);
    assert(c < Color::COLOR_NB);
    return Type((pt << 1) | c);
  }

  template <Color::Type C>
  struct PieceOfColor {
    static const Type Pawn = Type((PAWN << 1) | C);
    static const Type Knight = Type((KNIGHT << 1) | C);
    static const Type Bishop = Type((BISHOP << 1) | C);
    static const Type Rook = Type((ROOK << 1) | C);
    static const Type Queen = Type((QUEEN << 1) | C);
    static const Type King = Type((KING << 1) | C);
  };

  inline PieceType piece_type(Type pc)
  {
    assert(pc < PIECE_NB);
    return PieceType(pc >> 1);
  }

  std::string to_str(PieceType pc);
  std::string to_str(Type pc);
}

namespace Castling {
  enum Right {
    NONE = 0,
    WHITE_OO = 1 << 0, // Bit #0 set
    WHITE_OOO = 1 << 1,  // Bit #1 Set
    WHITE_BOTH = WHITE_OO | WHITE_OOO, // Bit #0 and #1 set
    BLACK_OO = 1 << 2,  // Bit #2 set
    BLACK_OOO = 1 << 3,  // Bit #3 set
    BLACK_BOTH = BLACK_OO | BLACK_OOO, // Bit #2 and #3 set
    ALL = WHITE_BOTH | BLACK_BOTH, // Bits #0, #1, #2, #3 set
    CASTLING_RIGHT_NB = 16
  };

  inline Right operator~(const Right& cr)
  {
    assert(cr < CASTLING_RIGHT_NB);
    // TODO: Try using plain ~cr
    return Right(cr ^ 15);
  }
  inline Right& operator&=(Right& cr1, const Right& cr2)
  {
    assert(cr1 < CASTLING_RIGHT_NB);
    assert(cr2 < CASTLING_RIGHT_NB);
    return cr1 = Right(cr1 & cr2);
  }
  inline Right& operator|=(Right& cr1, const Right& cr2)
  {
    assert(cr1 < CASTLING_RIGHT_NB);
    assert(cr2 < CASTLING_RIGHT_NB);
    return cr1 = Right(cr1 | cr2);
  }

  const Right CastleRights[Square::SQ_NB] = {
    ~WHITE_OOO, ALL, ALL, ALL, ~WHITE_BOTH, ALL, ALL, ~WHITE_OO,
    ALL, ALL, ALL, ALL, ALL, ALL, ALL, ALL,
    ALL, ALL, ALL, ALL, ALL, ALL, ALL, ALL,
    ALL, ALL, ALL, ALL, ALL, ALL, ALL, ALL,
    ALL, ALL, ALL, ALL, ALL, ALL, ALL, ALL,
    ALL, ALL, ALL, ALL, ALL, ALL, ALL, ALL,
    ALL, ALL, ALL, ALL, ALL, ALL, ALL, ALL,
    ~BLACK_OOO, ALL, ALL, ALL, ~BLACK_BOTH, ALL, ALL, ~BLACK_OO
  };

  inline bool is_kingside_castle(Square::Type to)
  {
    return (to & 4) != 0;
  }

  template <Color::Type C>
  inline bool can_castle_OO(Right cr)
  {
    return (cr & (WHITE_OO << (C * 2))) != 0;
  }

  template <Color::Type C>
  inline bool can_castle_OOO(Right cr)
  {
    return (cr & (WHITE_OOO << (C * 2))) != 0;
  }
  std::string to_str(Right cr);
}

namespace Move {
  /*
     Yaka stores a single move in 16 bits:
     Bits 0 - 5 for the 'from' square
     Bits 6 - 11 for the 'to' square
     Bits 12 - 13 for flags
     Bits 14 - 15 for promotion piece minus 1
     */
  enum class Type : uint16_t { NONE };
  enum class Flags {
    NONE,
    ENPASSANT = 1 << 12,
    PROMOTION = 2 << 12,
    CASTLING = 3 << 12
  };
  const int MaxMoves = 256;

  template <Flags F = Flags::NONE, Piece::PieceType Promotion = Piece::KNIGHT>
  inline Type make_move(Square::Type from, Square::Type to)
  {
    assert(from < Square::SQ_NB);
    assert(to < Square::SQ_NB);
    return Type(int(from) | int(to << 6) | int(F) | ((Promotion - Piece::KNIGHT) << 14));
  }

  inline Square::Type from_sq(Type m) { return Square::Type(int(m) & 0x3F); }
  inline Square::Type to_sq(Type m) { return Square::Type((int(m) >> 6) & 0x3F); }
  inline Flags flags(Type m) { return Flags(int(m) & 0x3000); }
  inline bool is_move(Type m, Flags f) { return (int(m) & int(f)) ? 1 : 0; }

  // Returns the promotion piece for this move, given that this move is a promotion
  inline Piece::PieceType promotion_pc(Type m)
  {
    assert(is_move(m, Flags::PROMOTION));
    return Piece::PieceType((int(m) >> 14) + 1);
  }

  std::string to_str(Type m);
}

#define DECLARE_TYPE(T)                                                       \
inline T operator+(const T& t1, const T& t2) { return T(int(t1) + int(t2)); } \
inline T operator-(const T& t1, const T& t2) { return T(int(t1) - int(t2)); } \
inline T operator+(const T& t1, int t2) { return T(int(t1) + t2); }           \
inline T operator-(const T& t1, int t2) { return T(int(t1) - t2); }           \
inline T& operator++(T& t) { return t = T(int(t) + 1); }                      \
inline T& operator--(T& t) { return t = T(int(t) - 1); }

DECLARE_TYPE(Color::Type);
DECLARE_TYPE(Square::File);
DECLARE_TYPE(Square::Rank);
DECLARE_TYPE(Square::Type);
DECLARE_TYPE(Piece::Type);
DECLARE_TYPE(Piece::PieceType);
DECLARE_TYPE(Castling::Right);

#define LongLine "============================================================="
const depth_t MaxPly = 128;

#if defined(_MSC_VER) && defined(_WIN64) && defined(NDEBUG)
#define USE_INTRIN
#endif
#endif
