#ifndef INC_UCI_H_
#define INC_UCI_H_

#include "yaka.h"
#include "position.h"
#include "movegen.h"
#include "timer.h"
#include "evaluator.h"
#include "searcher.h"
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <cctype>
#include <map>

class UCI {
  using Token = std::string;
  using TokenList = std::vector < std::string > ;

  std::ostream& os; // Output Stream
  std::istream& is; // Input Stream
  TokenList token_list;
  Position pos;
  Searcher searcher;
  std::vector<key_t> hash_list;
public:
  UCI() : os(std::cout), is(std::cin), searcher(pos, os)
  {
    os << Program::info() << std::endl;
    ucinewgame();
  };
  ~UCI() {};
  void uci_loop();
  void handle_token(const Token& token);
  void uci() { os << Program::uci_info() << "uciok\n"; }
  void isready() { os << "readyok\n"; };
  void go() {};
  void ucinewgame() {
    hash_list.clear();
    pos.parse_fen(Program::StartFen);
    hash_list.push_back(pos.hash());
    searcher.reset();
  }
  void position();
  void stop() {};
  void quit() { std::exit(EXIT_SUCCESS); };

  void display() { os << pos.to_str() << std::endl; }
  void moves();
  void move(size_t idx = 1);
  void bench();
  void perft();
  void verify();
  void rgame();
  void eval();
  void flip();
  void test_eval();
  void test_search();
  void search();
  void see();

  void handle_error(const char * error_str, const Token& token)
  {
    os << error_str << ": '" << token << '\'' << std::endl;
  }
};

#endif
