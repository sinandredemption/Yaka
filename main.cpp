#include <iostream>
#include <ctime>
#include <random>
#include "bitboard.h"
#include "random.h"
#include "attacks.h"
#include "position.h"
#include "uci.h"
#include "movegen.h"
#include "misc.h"
#include "timer.h"

int main()
{
  using namespace std;
  Bitboard::init();
  Attacks::init();
  Zobrist::init();
  Scores::init();

  UCI uci;
  uci.uci_loop();
  system("PAUSE");
  return 0;
}