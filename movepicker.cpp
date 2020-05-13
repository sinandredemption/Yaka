#include "movepicker.h"
#include "movegen.h"

MovePicker::MovePicker(const Position& pos_) : pos(pos_) {
  reset();
}

void MovePicker::score_moves(ScoredMoveList& move_list, depth_t ply, Move::Type hash_move)
{
  size_t idx;
  for (idx = 0; move_list.mlist[idx] != Move::Type::NONE; ++idx) {
    const Move::Type m = move_list.mlist[idx];

    int score = 0;

    if (pos.piece(Move::to_sq(m)) != Piece::NONE) { // Capture, score by MVVLVA
      score = 8 * MVVLVA_Value[pos.piece(Move::to_sq(m))];
      score -= MVVLVA_Value[pos.piece(Move::from_sq(m))];
      if (Move::is_move(m, Move::Flags::PROMOTION))
        score += 32 * MVVLVA_Value[Move::promotion_pc(m) << 1];
      score *= 3;
    }
    else {  // Non capture, score by Killer and Relative History Heuristic
      score = killer_score(m, ply);
      if (UseHistoryHeuristics)
        score += rhh_score(m);
    }

    if (m == hash_move)
      score += 1000;

    move_list.score_list[idx] = score;
  }
}

Move::Type MovePicker::get_next_move(ScoredMoveList& move_list, size_t idx)
{
  int score, best_score = move_list.score_list[idx];
  size_t m = idx; // m = index of the best move
  for (size_t n = idx + 1; move_list.mlist[n] != Move::Type::NONE; ++n) {
    score = move_list.score_list[n];
    if (score > best_score) {
      best_score = score;
      m = n;
    }
  }

  if (idx != m) {
    std::swap(move_list.mlist[idx], move_list.mlist[m]);
    std::swap(move_list.score_list[idx], move_list.score_list[m]);
  }

  return move_list.mlist[idx];
}
