#ifndef INC_EVALUATOR_H_
#define INC_EVALUATOR_H_

#include "score.h"

class Position;
/// <summary>
/// Implementation of a position evaluator which scores
/// on the following factors:
/// 1. Pawns (doubled, isolated etc.)
/// 2. Pieces (Piece-Square scoring, bonuses for patterns etc.)
/// 3. King Safety (this is particularly interestingly implemented)
/// 4. Threat detection (bonuses for pieces attacking other pieces)
/// It uses "Tapered Evaluation" to return the final score.
/// </summary>
class Evaluator {
  const Position* pos;
  Score score;
  uint64_t passed_pawns[Color::COLOR_NB];
  uint64_t mobility_area[Color::COLOR_NB];
  uint64_t king_zone[Color::COLOR_NB];
  uint64_t attacks_by[Piece::PIECE_NB];
  uint64_t all_attacks[Color::COLOR_NB];
  int non_pawn_material;
public:
  Evaluator() {};
  Evaluator(const Position& pos_);
  ~Evaluator() {};

  template <Color::Type Us>
  Score eval_pawns();
  template <Color::Type Us, Piece::PieceType Pt>
  Score eval_pieces();
  template <Color::Type Us>
  Score eval_king_safety();
  template <Color::Type Us>
  Score eval_threats();
  template <Piece::Type Pc, bool Defended>
  inline void score_threat(uint64_t bb, Score& s);
  template <Color::Type Us>
  Score eval_passed_pawns();

  inline int interpolate_score(const Score& s, int stm);

  void to_str(std::stringstream& ss, Score s1, Score s2, std::string entity);
  std::string to_str();

  void init(const Position& pos_);
  int eval();

  static inline double to_cp(int s);
};

template <Piece::Type Pc, bool Defended>
inline void Evaluator::score_threat(uint64_t bb, Score& s)
{
  Square::Type sq;
  if (bb &= attacks_by[Pc]) do {
    sq = Bitboard::lsb(bb);
    s += Scores::ThreatBonus[Defended][Pc][Piece::piece_type(pos->piece(sq))][sq];
  } while (bb &= bb - 1);
}

inline double Evaluator::to_cp(int s)
{
  return s / 1000.;
}

inline int Evaluator::interpolate_score(const Score& s, int stm)
{
  stm = (-2 * stm) + 1; // 1 for White, -1 for black
  if (non_pawn_material > Score::MG_BOUND) return stm * s.mg;
  if (non_pawn_material < Score::EG_BOUND) return stm * s.eg;
  int sc = ((non_pawn_material - Score::EG_BOUND) * 128) / Score::INTERPOL;
  sc = (s.mg * sc) + (s.eg * (128 - sc));
  sc /= 128;
  sc *= stm;
  return sc;
}

#endif
