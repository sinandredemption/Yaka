#ifndef INC_ZOBRIST_H_
#define INC_ZOBRIST_H_

#include "yaka.h"

namespace Zobrist {
  const key_t SideHash = 0xE6CD8F029262BE63ULL;

  extern key_t PieceHash[Piece::PIECE_NB][Square::SQ_NB];
  extern key_t CastlingHash[Castling::CASTLING_RIGHT_NB];
  extern key_t EpHash[Square::SQ_NB];

  void init();
  inline key_t hash(Piece::Type pc, Square::Type from, Square::Type to)
  {
    return PieceHash[pc][from] ^ PieceHash[pc][to];
  }
}

#endif
