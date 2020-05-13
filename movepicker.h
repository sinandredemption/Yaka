#ifndef INC_RHHEURISTICS_H_
#define INC_RHHEURISTICS_H_

#include "yaka.h"
#include "position.h"
#include <cmath>

class MoveGen;

struct ScoredMove
{
  Move::Type move;
  int score;
};

typedef struct {
  Move::Type* mlist;
  int score_list[Move::MaxMoves];
} ScoredMoveList;

struct SortMoveList {
  bool operator()(const ScoredMove& m1, const ScoredMove& m2) const {
    return m1.score > m2.score;
  }
};

const bool UseHistoryHeuristics = false;
// Implementation of the relative history heuristics based move ordering
class MovePicker
{
  const Position& pos;

  int heuristics[Piece::PIECE_NB][Square::SQ_NB];
  Move::Type killer_list[MaxPly][2];
  
public:
  const int Scale = 64;
  const int KillerScore = 50;
  const depth_t MaxHistoryDepth = 5;
  const int MaxHistoryScore = 250;
  const int MVVLVA_Value[Piece::PIECE_NB] = { 1, 1, 2, 2, 2, 2, 3, 3, 5, 5 };

  MovePicker() = delete;
  MovePicker(const Position& pos_);
  ~MovePicker() {};

  inline void reset();

  inline void reg_beta_cutoff(ScoredMoveList& move_list,
    size_t idx, depth_t ply, depth_t depth);

  inline bool is_killer(Move::Type m, depth_t ply);
  inline int rhh_score(Move::Type m);
  inline int killer_score(Move::Type m, depth_t ply);

  void score_moves(ScoredMoveList& move_list, depth_t ply, Move::Type hash_move);
  Move::Type get_next_move(ScoredMoveList& move_list, size_t idx);
};

// Register a beta cutoff and update related tables
inline void MovePicker::reg_beta_cutoff(ScoredMoveList& move_list,
  size_t idx, depth_t ply, depth_t depth)
{
  Move::Type m = move_list.mlist[idx];
  if (pos.piece(Move::to_sq(m)) != Piece::NONE)
    return;

  assert(ply < MaxPly);

  // Register this move as a technically killer move for this ply
  if (m != killer_list[ply][0]) {
    killer_list[ply][1] = killer_list[ply][0];
    killer_list[ply][0] = m;
  }

  if (!UseHistoryHeuristics)
    return;

  // Register history heuristics for this move
  int& hi = heuristics[pos.piece(Move::from_sq(m))][Move::to_sq(m)];
  if (depth > MaxHistoryDepth)
    depth = MaxHistoryDepth;
  double x = log((double)depth);
  x *= x * x * (depth + 2);
  int score = (int)(x + 1);
  hi += score;
  if (hi > MaxHistoryScore)
    hi = MaxHistoryScore;

  // Register butterfly heuristics for previously tried moves
  for (int i = (int)idx - 1; i >= 0; --i) {
    Move::Type m = move_list.mlist[i];
    if (pos.piece(Move::to_sq(m)) == Piece::NONE) {
      heuristics[pos.piece(Move::from_sq(m))][Move::to_sq(m)] -= score;
    }
  }
}

// Get Relative History Heuristic score of the move
inline int MovePicker::rhh_score(Move::Type m)
{
  return heuristics[pos.piece(Move::from_sq(m))][Move::to_sq(m)];
}

inline bool MovePicker::is_killer(Move::Type m, depth_t ply) {
  return (m == killer_list[ply][0]) || (m == killer_list[ply][1]);
}

inline int MovePicker::killer_score(Move::Type m, depth_t ply)
{
  if (m != killer_list[ply][0]) {
    if (m != killer_list[ply][1])
      return 0;
    else
      return KillerScore;
  }
  else return KillerScore + (KillerScore / 10);
}

inline void MovePicker::reset() {
  std::memset(heuristics, 0, sizeof(heuristics));
  std::memset(killer_list, 0, sizeof(killer_list));
}

#endif
