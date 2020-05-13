#include "ttable.h"



TranspositionTable::TranspositionTable()
{
  entry = nullptr;
  log2size = modulo = 0;
}


TranspositionTable::~TranspositionTable()
{
  delete[] entry;
}

void TranspositionTable::clear() {
  std::memset(entry, 0, sizeof(Entry) * (1ULL << log2size));
  generation = 0;
}

void TranspositionTable::resize(size_t log2size_) {
  if (entry != nullptr)
    delete[] entry;

  log2size = log2size_;
  modulo = (1 << log2size) - 1;
  entry = new Entry[1ULL << log2size];
  clear();
}