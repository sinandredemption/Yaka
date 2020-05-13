#include "benchmarker.h"
#include "timer.h"
#include "position.h"
#include "movegen.h"
#include "evaluator.h"
#include <sstream>
#include <cmath>
#include <malloc.h>

Benchmarker::Benchmarker(Position& pos_, std::ostream& os_, std::istream& is)
  : pos(pos_), os(os_)
{
  std::string fen;
  while (is) {
    std::getline(is, fen);
    if (fen[0] == ';') continue;  // Comments start with ';'
    if (fen.size() < 10) continue;
    fen_list.push_back(fen);
  }
}

PerftBench::~PerftBench()
{
  if (PerftHash != nullptr)
    free(PerftHash);
}
void PerftBench::allocate_hash(int power2_size)
{
  PerftHash = (PerftHashTable*)calloc(1 << power2_size, sizeof(PerftHashTable));
  hash_size = (1 << power2_size) - 1;
}

template <bool Hash>
uint64_t PerftBench::perft(depth_t depth)
{
  uint64_t nodes = 0;
  if (Hash) {
    if (nodes = PerftHash[pos.hash() & hash_size].lookup(pos.hash(), depth))
      return nodes;
  }
  MoveGen movegen(pos);
  GameLine gl;
  for (Move::Type * mlist_ptr = movegen.begin(); *mlist_ptr != Move::Type::NONE; mlist_ptr++) {
    pos.make_move(*mlist_ptr, gl);
    nodes += depth <= 2 ? MoveGen(pos).size() : perft<Hash>(depth - 1);
    pos.unmake_move(*mlist_ptr, gl);
  }
  if (Hash) {
    PerftHash[pos.hash() & hash_size].store(pos.hash(), depth, nodes);
  }
  return nodes;
}

uint64_t PerftBench::do_perft(depth_t depth)
{
  Timer timer;
  timer.start();
  uint64_t nodes = 0;
  MoveGen movegen(pos);
  GameLine gl;
  for (Move::Type * mlist_ptr = movegen.begin(); *mlist_ptr != Move::Type::NONE; mlist_ptr++) {
    pos.make_move(*mlist_ptr, gl);
    uint64_t subnodes = depth <= 2 ? MoveGen(pos).size() : PerftHash == nullptr ?
      perft<false>(depth - 1) : perft<true>(depth - 1);
    nodes += subnodes;
    os << Move::to_str(*mlist_ptr) << ": " << subnodes << std::endl;
    if (UpdateCout)
      std::cerr.put('.');
    pos.unmake_move(*mlist_ptr, gl);
  }
  timer.stop();
  unsigned knps = unsigned(nodes / timer.get_elapsed_ms());
  os << "Took " << timer << " for " << nodes << " nodes, " << knps << " KNPS\n";
  if (UpdateCout)
    std::cout << "\nTook " << timer << " for " << nodes << " nodes, " << knps << " KNPS\n";
  return nodes;
}

void PerftBench::benchmark(depth_t depth)
{
  size_t idx;
  for (idx = 0; idx < fen_list.size(); ++idx) {
    os << "Position #" << idx + 1 << ": " << fen_list[idx] << '\n';
    os << LongLine << '\n';
    if (UpdateCout)
      std::cout << "Position #" << idx + 1 << ": " << fen_list[idx] << '\n' << LongLine << '\n';
    pos.parse_fen(fen_list[idx]);
    do_perft(depth);
    os << LongLine << "\n\n";
    if (UpdateCout)
      std::cout.put('\n');
  }
}

void PerftBench::verify(depth_t depth)
{
  size_t idx;
  for (idx = 0; idx < fen_list.size(); ++idx) {
    std::string temp;
    uint64_t expect, actual;
    std::stringstream ss(fen_list[idx]);
    ss >> temp >> temp >> temp >> temp >> temp >> temp >> expect;
    os << "Position #" << idx + 1 << ": " << fen_list[idx] << '\n';
    os << LongLine << '\n';
    if (UpdateCout)
      std::cout << "Position #" << idx + 1 << ": " << fen_list[idx] << '\n' << LongLine << '\n';
    pos.parse_fen(fen_list[idx]);
    actual = do_perft(depth);

    if (actual != expect) {
      os << "ERROR: Expected " << expect << " but got " << actual << " nodes.\n";
      std::cout << "ERROR: Expected " << expect << " but got " << actual << " nodes.\n";
    }

    os << LongLine << "\n\n";
    if (UpdateCout)
      std::cout.put('\n');
  }
}

template <bool Debug>
void EvalDebugger::eval()
{
  Evaluator ev;
  ev.init(pos);
  if (!Debug)
    int s = ev.eval();
  else {
    // Test evaluation symmetry
    int s1 = ev.eval();
    Position flipped(pos.flip());
    ev.init(flipped);
    int s2 = ev.eval();
    if (s1 != s2) {
      os << "\nERROR: " << pos.to_fen() << std::endl;
      os << "Expected: " << s1 << " but got: " << s2 << std::endl;
    }
    if (UpdateCout && (s1 != s2))
      std::cerr.put('\a').put('!');
  }
}

template <bool Debug>
uint64_t EvalDebugger::test_eval(depth_t depth)
{
  if (depth == 0) {
    eval<Debug>();
    return 1;
  }

  MoveGen movegen(pos);
  uint64_t nodes = 0;
  GameLine gl;
  for (Move::Type * mlist_ptr = movegen.begin(); *mlist_ptr != Move::Type::NONE; ++mlist_ptr) {
    pos.make_move(*mlist_ptr, gl);
    eval<Debug>();
    nodes += test_eval<Debug>(depth - 1);
    pos.unmake_move(*mlist_ptr, gl);
  }

  return nodes;
}

void EvalDebugger::benchmark(depth_t depth, bool debug)
{
  size_t idx;
  uint64_t nodes;

  os << (debug ? "Debugging " : "Testing ") << fen_list.size() << " positions...\n";
  if (UpdateCout)
    std::cout << (debug ? "Debugging " : "Testing ") << fen_list.size() << " positions...\n";

  uint64_t total_nodes = 0;
  Timer total_time;
  total_time.start();
  for (idx = 0; idx < fen_list.size(); ++idx) {
    if (UpdateCout)
      std::cerr << "Position " << idx + 1 << "/" << fen_list.size() << "...\n";
    os << "Position #" << idx + 1 << ": " << fen_list[idx] << '\n';
    os << LongLine << '\n';
    pos.parse_fen(fen_list[idx]);

    Timer timer;
    timer.start();
    nodes = debug ? test_eval<true>(depth) : test_eval<false>(depth);
    total_nodes += nodes;
    timer.stop();

    os << "Took " << timer << " for " << nodes << " nodes, " << nodes / timer.get_elapsed_ms() << " KNPS.\n";

    os << LongLine << "\n\n";
  }
  total_time.stop();
  os << "Took " << total_time << " for " << total_nodes
    << " nodes, " << total_nodes / total_time.get_elapsed_ms() << " KNPS.\n";
  std::cout << "\nDone\n";
}

void SearchTest::test(depth_t depth, size_t hsize)
{
  searcher.ttable.resize(hsize);
  if (UpdateCout)
    std::cout << "Testing " << fen_list.size() << " positions...\n";

  uint64_t total_nodes = 0;
  double bf = 0;
  HashList hl;
  for (size_t idx = 0; idx < fen_list.size(); ++idx) {
    if (UpdateCout)
      std::cerr << "Position " << idx + 1 << "/" << fen_list.size() << "...\n";

    hl.clear();
    os << "Position #" << idx + 1 << ": " << fen_list[idx] << '\n';
    os << LongLine << '\n';

    pos.parse_fen(fen_list[idx]);
    uint64_t nodes = searcher.search(depth, hl);
    searcher.ttable.clear();

    total_nodes += nodes;
    double b = pow((double) nodes, 1. / depth);
    os << "Branching factor: " << b << "\n\n";
    bf += b;
  }
  os << "\n\nTotal nodes: " << total_nodes
    << "\tAverage Branching factor: " << bf / fen_list.size();
  os << std::endl;
  std::cout << "\nDone\n";
}
