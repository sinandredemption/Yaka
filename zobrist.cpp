#include "zobrist.h"
#include "random.h"

namespace Zobrist {
  key_t PieceHash[Piece::PIECE_NB][Square::SQ_NB];
  key_t CastlingHash[Castling::CASTLING_RIGHT_NB] = { 0 };
  key_t EpHash[Square::SQ_NB] = { 0 };

  void init()
  {
    for (Piece::Type pc = Piece::WHITE_PAWN; pc <= Piece::BLACK_KING; ++pc) {
      for (Square::Type sq = Square::A1; sq < Square::SQ_NB; ++sq) {
        PieceHash[pc][sq] = Random::rand64();
        EpHash[sq] ^= Random::rand64();
        CastlingHash[sq % Castling::CASTLING_RIGHT_NB] ^= Random::rand64();
      }
    }
  }
}