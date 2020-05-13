#ifndef TTABLE_H_
#define TTABLE_H_
#include "yaka.h"
#include "score.h"

enum class TTScoreType {
  EmptyScore,
  ExactScore,
  AlphaBound, // Actual score might be lesser
  BetaBound   // Actual score might be greater
};

class TranspositionTable
{
  size_t log2size;
  size_t modulo;  // index of a position in table = <hash> mod (modulo + 1)
public:
  static inline int to_tt_score(int s, depth_t ply)
  {
    if (Score::is_mate_score(s)) s += ply;
    else if (Score::is_mate_score(-s)) s -= ply;

    return s;
  }

  static inline int from_tt_score(int s, depth_t ply)
  {
    if (Score::is_mate_score(s)) s -= ply;
    else if (Score::is_mate_score(-s)) s += ply;

    return s;
  }

  struct Entry
  {
    key_t key;
    uint8_t depth;
    int16_t score, eval_score;
    uint16_t best_move;
    // 'other' stores the TTScoreType and the generation of this entry:
    // <TTScoreType> = other & 0x03
    // <generation>  = other >> 2
    uint8_t other;

    key_t hash() const { return key; }
    key_t& hash() { return key; }

    depth_t get_depth() const { return (depth_t)depth; }
    void set_depth(depth_t depth_) { depth = (uint8_t) depth_; }

    int get_score(int ply) const { return from_tt_score(score, ply); }
    void set_score(int s) { score = (int16_t) s; }
    int get_eval() const { return eval_score; }
    void set_eval(int s) { eval_score = (int16_t) s; }

    Move::Type get_best_move() const { return (Move::Type) best_move; }
    void set_best_move(Move::Type m) { best_move = (uint16_t) m; }

    TTScoreType get_type() const {
      return TTScoreType(other & 0x03);
    }
    void set_type(TTScoreType type) {
      other = (other & 0xFC) | uint8_t(type);
    }
    
    int get_generation() const {
      return other >> 2;
    }
    void set_generation(uint8_t gen) {
      assert(gen < (1 << 6));
      other = (other & 0x03) | (gen << 2);
    }
  };
private:
  Entry* entry;
  uint8_t generation;
public:
  TranspositionTable();
  ~TranspositionTable();
  void clear();
  void resize(size_t log2size_);
  inline void inc_gen();
  inline void record(key_t hash, depth_t depth, int score,
    int eval_score, Move::Type best_move, TTScoreType type, depth_t ply);
  inline void record_eval(key_t hash, int eval_score);
  inline Entry* probe(key_t hash);
};

inline void TranspositionTable::inc_gen() {
  generation = (generation + 1) & 63;
}

inline void TranspositionTable::record(key_t hash, depth_t depth, int score,
  int eval_score, Move::Type best_move, TTScoreType type, depth_t ply)
{
  Entry& e = entry[hash & modulo];

  assert(e.get_generation() <= generation);

  bool do_record = (e.get_type() == TTScoreType::EmptyScore);
  if (!do_record) {
    if (e.get_generation() != generation)
      do_record = true;
    if ((type != TTScoreType::ExactScore) && (e.get_type() == TTScoreType::ExactScore))
      do_record = false;
    if (depth <= e.get_depth())
      do_record = false;
  }

  if (do_record) {
    e.hash() = hash;
    e.set_depth(depth);
    e.set_score(to_tt_score(score, ply));
    e.set_eval(eval_score);
    e.set_best_move(best_move);
    e.set_type(type);
    e.set_generation(generation);
  }
}

inline TranspositionTable::Entry* TranspositionTable::probe(key_t hash)
{
  Entry* e;
  if ((e = &entry[hash & modulo])->hash() == hash) {
    assert(e->get_type() != TTScoreType::EmptyScore);
    return e;
  }
  else return nullptr;

  return e;
}

inline void TranspositionTable::record_eval(key_t hash, int eval_score) {
  size_t idx = hash & modulo;
  if (entry[idx].get_depth() >= 0)
    return;
  assert((entry[idx].get_type() == TTScoreType::EmptyScore) ||
    (hash != entry[idx].hash()));

  entry[idx].set_eval(eval_score);
  entry[idx].set_score(eval_score);
  entry[idx].set_type(TTScoreType::ExactScore);
}

#endif
