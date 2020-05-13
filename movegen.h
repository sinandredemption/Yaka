#ifndef INC_MOVEGEN_H_
#define INC_MOVEGEN_H_
#include "yaka.h"
#include "bitboard.h"

class Position;

class MoveGen {
  Move::Type mstack[Move::MaxMoves];
  Move::Type * end;
  const Position& pos;
  const uint64_t NotPinned;
public:
  MoveGen() = delete;
  MoveGen(const Position& pos_);
  ~MoveGen() {}

  Move::Type* begin()
  {
    return mstack;
  }

  Move::Type operator[](size_t i) const
  {
    return mstack[i];
  }

  Move::Type& operator[](size_t i)
  {
    return mstack[i];
  }

  size_t size()
  {
    return end - mstack;
  }

  template <Move::Flags Flag = Move::Flags::NONE, Piece::PieceType Prom = Piece::KNIGHT>
  void add_move(Square::Type from, Square::Type to)
  {
    *end++ = Move::make_move<Flag, Prom>(from, to);
  }

  void add_moves(Square::Type from, uint64_t attacks)
  {
    if (attacks) do
      *end++ = Move::make_move(from, Bitboard::lsb(attacks));
    while (attacks &= attacks - 1);
  }

  template <int Dir>
  void add_promotions(uint64_t pawns, uint64_t target)
  {
    pawns = Bitboard::shift_bb<Dir>(pawns) & target;
    if (pawns) do {
      Square::Type to = Bitboard::lsb(pawns);
      Square::Type from = to - Dir;
      add_move<Move::Flags::PROMOTION, Piece::QUEEN>(from, to);
      add_move<Move::Flags::PROMOTION, Piece::KNIGHT>(from, to);
      add_move<Move::Flags::PROMOTION, Piece::ROOK>(from, to);
      add_move<Move::Flags::PROMOTION, Piece::BISHOP>(from, to);
    } while (pawns &= pawns - 1);
  }

  template <int Dir>
  void add_pawn_captures(uint64_t pawns, uint64_t target)
  {
    pawns = Bitboard::shift_bb<Dir>(pawns) & target;
    if (pawns) do {
      Square::Type to = Bitboard::lsb(pawns);
      add_move(to - Dir, to);
    } while (pawns &= pawns - 1);
  }

  template <bool Capture, int Dir, Color::Type Us>
  void add_pinned_pawn_move(const uint64_t b)
  {
    assert(!Bitboard::more_than_one(b));
    if (Capture && (b & (Bitboard::Rank8Mask >> (Us * 8 * 7)))) {
      Square::Type to = Bitboard::lsb(b);
      Square::Type from = to - Dir;
      add_move<Move::Flags::PROMOTION, Piece::QUEEN>(from, to);
      add_move<Move::Flags::PROMOTION, Piece::KNIGHT>(from, to);
      add_move<Move::Flags::PROMOTION, Piece::ROOK>(from, to);
      add_move<Move::Flags::PROMOTION, Piece::BISHOP>(from, to);
    } else if (b) {
      Square::Type to = Bitboard::lsb(b);
      add_move(to - Dir, to);
    }
  }

  template <Color::Type Us>
  void gen_pawn_moves(const uint64_t Target = Bitboard::Universe);
  template <Color::Type Us, bool InCheck = false>
  void gen_king_moves(uint64_t target);
  void gen_knight_moves(uint64_t target);
  template <Piece::PieceType P> void gen_slider_moves(uint64_t pieces, uint64_t target);
  template <Color::Type Us> void generate_all(const uint64_t Target)
  {
    using OurPieces = Piece::PieceOfColor < Us > ;
    gen_slider_moves<Piece::ROOK>(pos.pieces(OurPieces::Rook, OurPieces::Queen), Target);
    gen_slider_moves<Piece::BISHOP>(pos.pieces(OurPieces::Bishop, OurPieces::Queen), Target);
    gen_knight_moves(Target);
  }

  template <Color::Type Us> void generate_evasions()
  {
    assert(Us == pos.side_to_move());
    gen_king_moves<Us, true>(~pos.pieces(Us));
    if (!Bitboard::more_than_one(pos.checkers())) {
      // Not a double check, there might be other possible evasions
      Square::Type sq = Bitboard::lsb(pos.checkers());
      uint64_t target = Attacks::SqBetween[pos.king_square(Us)][sq];
      target |= Bitboard::sq_mask(sq);
      using OurPieces = Piece::PieceOfColor < Us > ;
      gen_slider_moves<Piece::ROOK>(pos.pieces(OurPieces::Rook, OurPieces::Queen) & NotPinned, target);
      gen_slider_moves<Piece::BISHOP>(pos.pieces(OurPieces::Bishop, OurPieces::Queen) & NotPinned, target);
      gen_knight_moves(target);
      gen_pawn_moves<Us>(target);
    }
  }

  void generate();
  std::string to_str() const;
};

#endif
