#ifndef INC_SEARCHER_H_
#define INC_SEARCHER_H_

#include "yaka.h"
#include "position.h"
#include "evaluator.h"
#include "movepicker.h"
#include "ttable.h"
#include <iostream>
#include <algorithm>

typedef std::vector<key_t> HashList;

class Searcher
{
  Position& pos;
  uint64_t nodes, tthits;
  std::ostream &os;
  HashList hash_list;
  depth_t game_ply;
  MovePicker movepicker;
  bool allow_nullmove[MaxPly];

  typedef std::vector<ScoredMove> RootMoveList;
public:
  const depth_t NullMoveMinDepth = 3;
  const depth_t NullMovePruningDepth = 2;
  const int AspirationWindowSize = 40;
  TranspositionTable ttable;

  Searcher() = delete;
  Searcher(Position& pos_, std::ostream &os_) :
    pos(pos_), os(os_), movepicker(pos), nodes(0), tthits(0) {}
  ~Searcher() {};

  inline void reset();
  inline bool is_draw(depth_t ply);
  uint64_t search(depth_t depth, const HashList& hl);
  int alpha_beta(int alpha, int beta, depth_t depth, depth_t ply);
  int qsearch(int alpha, int beta, int depth, int ply);
  std::string extract_pv(Move::Type root_move);
};

inline void Searcher::reset() {
  movepicker.reset();
}

// Test if the position is draw by insuffecient material or repetition
// Stalemates and fifty-move rule is handled in the search
inline bool Searcher::is_draw(depth_t ply) {
  // Handle draws due to insufficient material
  if (!(pos.pawns(Color::WHITE) || pos.pawns(Color::BLACK))) {
    using namespace Piece;
    if (!(pos.pieces(WHITE_ROOK) || pos.pieces(BLACK_ROOK)
      || pos.pieces(WHITE_QUEEN) || pos.pieces(BLACK_QUEEN))) {
      count_t n_pieces = Bitboard::bit_count(pos.all_pieces());
      // We know there are no queens or rooks, so if there are less than
      // four pieces on the board (including two kings), then we know it's
      // a draw
      if (n_pieces < 4)
        return true;

      // If there are no knights on the board, then if the bishops are on
      // the same color, it's a draw
      if (!(pos.pieces(WHITE_KNIGHT) || pos.pieces(BLACK_KNIGHT))) {
        uint64_t b = pos.pieces(WHITE_BISHOP, BLACK_BISHOP);
        if (((b & Bitboard::LightSquares) == 0) || ((b & Bitboard::DarkSquares) == 0))
          return true;
      }
    }
  }

  // Test for draw by repetition
  int n_reps = 1;
  int stop = std::max(int(game_ply + ply) - pos.half_move(), 0);
  for (int i = int(game_ply + ply) - 2; i >= stop; i -= 2) {
    if (pos.hash() == hash_list[i]) {
      n_reps++;
      if (n_reps >= 3)
        return true;
    }
  }

  return false;
}

#endif
