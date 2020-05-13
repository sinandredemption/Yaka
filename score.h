#ifndef INC_SCORE_H_
#define INC_SCORE_H_

#include "yaka.h"


struct Score {
  enum {
    MIDGAME, ENDGAME, MG_BOUND = 75000, EG_BOUND = 19000, INTERPOL = MG_BOUND - EG_BOUND,
    MATE_SCORE = 1 << 30, UNKNOWN_SCORE = MATE_SCORE + 3141, DRAW_SCORE = 0
  };
  
  static inline bool is_mate_score(int s) {
    return s >= MATE_SCORE - 1000;
  }

  int mg, eg;
  Score() {};
  Score(int mg_, int eg_) : mg(mg_), eg(eg_) {}

  Score& operator+=(const Score& other)
  {
    mg += other.mg;
    eg += other.eg;
    return *this;
  }

  Score& operator-=(const Score& other)
  {
    mg -= other.mg;
    eg -= other.eg;
    return *this;
  }

#define DEFINE_OPERATOR(x) Score& operator x (int s) { mg x s; eg x s; return *this; }
  DEFINE_OPERATOR(+= );
  DEFINE_OPERATOR(-= );
#undef DEFINE_OPERATOR
#define DEFINE_OPERATOR(x) Score operator x (int s) { return { mg x s, eg x s }; }
  DEFINE_OPERATOR(+);
  DEFINE_OPERATOR(*);
#undef DEFINE_OPERATOR
#define DEFINE_OPERATOR(x) Score operator x (const Score& s) { return { mg - s.mg, eg - s.eg }; }
  DEFINE_OPERATOR(+);
  DEFINE_OPERATOR(-);
  inline void negate()
  {
    mg = -mg;
    eg = -eg;
  }

  ~Score() {};
};

namespace Scores {
  const int KingValue = 1000000;
  extern Score PawnValue;
  extern Score KnightValue;
  extern Score BishopValue;
  extern Score RookValue;
  extern Score QueenValue;

  extern Score PieceVal[Piece::PIECE_TYPE_NB];
  extern Score PtSqTab[Piece::PIECE_TYPE_NB][Square::SQ_NB];
  extern Score PcSqTab[12][Square::SQ_NB];

  // Pawns
  extern Score BackwardPawnPenalty;
  extern Score DoubledPawnPenalty[Square::FILE_NB];
  extern Score IsolatedPawnPenalty[Square::FILE_NB];
  extern Score ConnectedPawnBonus[Square::RANK_NB];
  extern Score WeakPawnPenalty;
  extern Score PassedPawnBonus[Square::RANK_NB];
  extern Score SupportedPasserBonus;
  extern Score PasserCanAdvance;
  extern Score PasserBlocked;
  extern Score PasserNotBlocked;
  extern Score PasserPathClear;

  extern Score MobilityBonus[4][28];
  extern Score OutpostBonus[2][Square::SQ_NB];

  extern Score BishopPairBonus;
  extern Score RookOnOpenFile;
  extern Score RookOnSemiOpenFile;

  // King Safety
  extern int SafetyTable[100];
  // ThreatBonus[defended][attacker][attacked piece][square]
  extern Score ThreatBonus[2][12][Piece::PIECE_TYPE_NB][Square::SQ_NB];
  extern Score HangingPenalty;
  void init();
}

#endif
