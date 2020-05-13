#ifndef INC_BENCHMARKER_H_
#define INC_BENCHMARKER_H_
#include "yaka.h"
#include "searcher.h"
#include <string>
#include <vector>
#include <iostream>

class Position;
class Benchmarker {
protected:
  std::vector<std::string> fen_list;
  Position& pos;
  std::ostream& os;
public:
  Benchmarker() = delete;
  Benchmarker(Position& pos_, std::ostream& os_) : pos(pos_), os(os_) {}
  Benchmarker(Position& pos_, std::ostream& os_, std::istream& is);
  ~Benchmarker(void) {}
};

class PerftBench : public Benchmarker {
  struct PerftHashTable {
    key_t hash_key;
    uint64_t data;

    void store(key_t hash, depth_t d, uint64_t nodes)
    {
      if (nodes < (data >> 6)) return;
      hash_key = hash;
      assert(nodes < (1ULL << 58));
      data = (nodes << 6) | d;
    }

    uint64_t lookup(key_t hash, depth_t d)
    {
      if ((hash == hash_key) && ((data & 63) == d))
        return data >> 6;
      else return 0;
    }
  } *PerftHash;
  size_t hash_size;
  const bool UpdateCout;
public:
  PerftBench() = delete;
  PerftBench(Position& pos_, std::ostream& os_)
    : Benchmarker(pos_, os_), UpdateCout(false) { PerftHash = nullptr; }
  PerftBench(Position& pos_, std::ostream& os_, std::istream& is)
    : Benchmarker(pos_, os_, is), UpdateCout(false) { PerftHash = nullptr; }
  PerftBench(Position& pos_, std::ostream& os_, std::istream& is, bool update)
    : Benchmarker(pos_, os_, is), UpdateCout(update) { PerftHash = nullptr; }
  ~PerftBench();

  template <bool Hash>
  uint64_t perft(depth_t depth);
  uint64_t do_perft(depth_t depth);
  void benchmark(depth_t depth);
  void verify(depth_t depth);
  void allocate_hash(int power2_size);
};

class EvalDebugger : public Benchmarker {
  const bool UpdateCout;
public:
  EvalDebugger() = delete;
  EvalDebugger(Position& pos_, std::ostream& os_)
    : Benchmarker(pos_, os_), UpdateCout(false) {}
  EvalDebugger(Position& pos_, std::ostream& os_, std::istream& is)
    : Benchmarker(pos_, os_, is), UpdateCout(false) {}
  EvalDebugger(Position& pos_, std::ostream& os_, std::istream& is, bool update)
    : Benchmarker(pos_, os_, is), UpdateCout(update) {}
  ~EvalDebugger() {};

  template <bool Debug>
  void eval();
  template <bool Debug>
  uint64_t test_eval(depth_t depth);
  void benchmark(depth_t depth, bool debug = false);
};

class SearchTest : public Benchmarker {
  const bool UpdateCout;
  Searcher searcher;
public:
  SearchTest() = delete;
  SearchTest(Position& pos_, std::ostream& os_)
    : Benchmarker(pos_, os_), UpdateCout(false), searcher(pos_, os_) {}
  SearchTest(Position& pos_, std::ostream& os_, std::istream& is)
    : Benchmarker(pos_, os_, is), UpdateCout(false), searcher(pos_, os_) {}
  SearchTest(Position& pos_, std::ostream& os_, std::istream& is, bool update)
    : Benchmarker(pos_, os_, is), UpdateCout(update), searcher(pos_, os_) {}
  ~SearchTest() {};

  void test(depth_t depth, size_t hsize);
};
#endif
