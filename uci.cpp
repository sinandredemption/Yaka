#include "uci.h"
#include "misc.h"
#include "benchmarker.h"

void UCI::uci_loop()
{
  using namespace std;
  Token t;
  while (is) {
    getline(is, t);
    token_list.clear();
    Misc::split_string(t, &token_list);
    if (token_list.size() != 0)
      handle_token(token_list[0]);
  }
  quit();
}

void UCI::handle_token(const Token& token)
{
  if (token == "uci")             uci();
  else if (token == "isready")    isready();
  else if (token == "go")         go();
  else if (token == "ucinewgame") ucinewgame();
  else if (token == "position")   position();
  else if (token == "stop")       stop();
  else if (token == "quit")       quit();
  else if (token == "d")          display();

  else if (token == "moves")      moves();
  else if (token == "move")       move();
  else if (token == "perft")      perft();
  else if (token == "bench")      bench();
  else if (token == "verify")     verify();
  else if (token == "rgame")      rgame();
  else if (token == "eval")       eval();
  else if (token == "flip")       flip();
  else if (token == "testeval")   test_eval();
  else if (token == "testsearch") test_search();
  else if (token == "search")     search();
  else if (token == "SEE")        see();
  else                            handle_error("Unknown Token", token);
}

void UCI::position()
{
  size_t idx = 2;
  std::string fen_str;
  if (token_list[1] == "fen") {
    while (idx < token_list.size() && token_list[idx] != "moves")
      fen_str += token_list[idx++] + ' ';
  } else if (token_list[1] == "startpos")
    fen_str = Program::StartFen;
  else if (token_list[1] == "kiwipete") // Useful for debugging
    fen_str = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
  else {
    handle_error("Unknown Token", token_list[1]);
    return;
  }

  hash_list.clear();
  pos.parse_fen(fen_str);
  hash_list.push_back(pos.hash());

  if (token_list.size() > idx && token_list[idx++] == "moves") {
    move(idx);
  }
  searcher.reset();
  searcher.ttable.inc_gen();
}

void UCI::moves()
{
  os << MoveGen(pos).to_str() << std::endl;
}

void UCI::move(size_t idx)
{
  if (idx >= token_list.size()) {
    handle_error("No move specified", "");
    return;
  }
  MoveGen movegen(pos);
  for (Move::Type * mlist_ptr = movegen.begin(); *mlist_ptr != Move::Type::NONE; mlist_ptr++) {
    if (Move::to_str(*mlist_ptr) == token_list[idx]) {
      pos.make_move(*mlist_ptr, GameLine());
      hash_list.push_back(pos.hash());
      if (idx < (token_list.size() - 1)) move(idx + 1);
      return;
    }
  }
  handle_error("Unknown move", token_list[idx]);
}

void UCI::perft()
{
  if (token_list.size() < 2) {
    handle_error("No Depth Provided", token_list[0]);
    return;
  }
  depth_t depth = Misc::convert_to <depth_t>(token_list[1]);
  if (token_list.size() > 2) {
    PerftBench pb(pos, os);
    int power_two_size = Misc::convert_to<int>(token_list[2]);
    if (power_two_size >= 32) handle_error("Too Large Size", token_list[2]);
    else {
      pb.allocate_hash(power_two_size);
      pb.do_perft(depth);
    }
  } else PerftBench(pos, os).do_perft(depth);
}

void UCI::bench()
{
  if (token_list.size() != 4) {
    os << "Usage: bench <depth> <input filename (without spaces)> <output file>\n";
    return;
  }
  depth_t depth = Misc::convert_to<depth_t>(token_list[1]);
  std::ifstream input(token_list[2]);
  std::ofstream output(token_list[3]);
  PerftBench(pos, output, input, true).benchmark(depth);
}

void UCI::verify()
{
  if (token_list.size() != 4) {
    os << "Usage: verify <depth> <input filename (without spaces)> <output file>\n";
    return;
  }
  depth_t depth = Misc::convert_to<depth_t>(token_list[1]);
  std::ifstream input(token_list[2]);
  std::ofstream output(token_list[3]);
  PerftBench(pos, output, input, true).verify(depth);
}

void UCI::rgame()
{
  Random::s *= Timer::now();
  ucinewgame();
  size_t i = 2;
  do {
    MoveGen movegen(pos);
    if (movegen.size() == 0) break;
    if (i % 2 == 0) os << i / 2 << ". ";
    Move::Type m = movegen[Random::rand_int(movegen.size())];
    os << Move::to_str(m) << ' ';
    pos.make_move(m, GameLine());
    ++i;
  } while (true);
  os << std::endl;
}

void UCI::eval()
{
  Evaluator ev(pos);
  os << ev.to_str();
}

void UCI::flip()
{
  pos = pos.flip();
}

void UCI::test_eval()
{
  if (token_list.size() != 4) {
    os << "Usage: testeval <depth> <input filename (without spaces)> <output file>\n";
    return;
  }
  depth_t depth = Misc::convert_to<depth_t>(token_list[1]);
  std::ifstream input(token_list[2]);
  std::ofstream output(token_list[3]);
  EvalDebugger(pos, output, input, true).benchmark(depth, Program::Debugging);
}

void UCI::search() {
  if (token_list.size() < 2) {
    handle_error("The search depth was not provided", token_list[0]);
    return;
  }
  if (token_list.size() < 3) {
    handle_error("The hash table size was not provided", token_list[0]);
    return;
  }

  depth_t depth = Misc::convert_to<depth_t>(token_list[1]);
  int hsize = Misc::convert_to<int>(token_list[2]);
  searcher.ttable.resize(hsize);
  searcher.search(depth, hash_list);
}

void UCI::test_search()
{
  if (token_list.size() != 5) {
    os << "Usage: testsearch <depth> <hash table size> <input filename (without spaces)> <output file>\n";
    return;
  }

  depth_t depth = Misc::convert_to<depth_t>(token_list[1]);
  size_t hsize = Misc::convert_to<size_t>(token_list[2]);
  std::ifstream input(token_list[3]);
  std::ofstream output(token_list[4]);

  SearchTest(pos, output, input, true).test(depth, hsize);
}

void UCI::see() {
  if (token_list.size() != 2) {
    os << "Usage: SEE <move>" << std::endl;
    return;
  }

  MoveGen movegen(pos);
  for (Move::Type * mlist_ptr = movegen.begin(); *mlist_ptr != Move::Type::NONE; mlist_ptr++) {
    if (Move::to_str(*mlist_ptr) == token_list[1]) {
      //os << pos.SEE<Color::WHITE>(*mlist_ptr);
      os << pos.see(pos.side_to_move(), *mlist_ptr) << std::endl;
      return;
    }
  }

  handle_error("Unknown move", token_list[1]);
}