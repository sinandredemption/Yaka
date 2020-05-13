#include "movegen.h"
#include "position.h"
#include "attacks.h"
#include <sstream>

void MoveGen::generate() {
  if (pos.checkers())
    pos.side_to_move() == Color::WHITE ? generate_evasions<Color::WHITE>()
    : generate_evasions<Color::BLACK>();
  else {
    const uint64_t Target = ~pos.pieces(pos.side_to_move());
    if (pos.side_to_move() == Color::WHITE) {
      generate_all<Color::WHITE>(Target);
      gen_king_moves<Color::WHITE>(Target);
      gen_pawn_moves<Color::WHITE>();
    }
    else {
      generate_all<Color::BLACK>(Target);
      gen_king_moves<Color::BLACK>(Target);
      gen_pawn_moves<Color::BLACK>();
    }
  }
  *end = Move::Type::NONE;
}

MoveGen::MoveGen(const Position& pos_) :
  end(mstack), pos(pos_), NotPinned(~pos.pinned())
{
  generate();
}

template <Color::Type Us>
void MoveGen::gen_pawn_moves(const uint64_t Target)
{
  using namespace Bitboard;
  const uint64_t FreeSquares = ~pos.all_pieces();
  const uint64_t CapSquares = pos.pieces(~Us) & Target;

  const int One = Us == Color::WHITE ? 1 : -1;

  uint64_t b1, b2, pawns; // Temporary Bitboards

  // We mask the pawns that are on Rank 7 for white or Rank 2 for black.
  // These pawns can only promote. Also, we remove pinned pawns from the mask
  // because pinned pawns can never promote
  if (Target & (Rank7Mask >> (Us * 8 * 5)))
    if (pawns = pos.pawns(Us) & (Rank7Mask >> (Us * 8 * 5)) & NotPinned) {
      add_promotions<9 * One>(pawns, CapSquares);  // Right Direction
      add_promotions<7 * One>(pawns, CapSquares);  // Left  Direction
      add_promotions<8 * One>(pawns, FreeSquares & Target); // Non-capture, up
    }

  // We mask out the pawns that are not on Rank 7 for white or
  // Rank 2 for black. These pawns won't promote. Also, pinned pawn
  // capture generation is handled later
  const uint64_t Pawns = pos.pawns(Us) & ~(Rank7Mask >> (Us * 8 * 5));
  pawns = Pawns & NotPinned;
  // Now generate pawn captures.
  add_pawn_captures<9 * One>(pawns, CapSquares);
  add_pawn_captures<7 * One>(pawns, CapSquares);

  // Pinned pawn move generation
  if (Target == Universe) { // Not in check
    pawns = pos.pawns(Us) & pos.pinned();

    b1 = shift_bb<8 * One>(pawns & Pawns) & FreeSquares & Attacks::FileMaskEx[pos.king_square(Us)];
    add_pinned_pawn_move<false, 8 * One, Us>(b1 & Target);
    add_pinned_pawn_move<false, 16 * One, Us>(shift_bb<8 * One>(b1 & (Rank3Mask << (Us * 3 * 8)))
                                              & FreeSquares & Target);

    // Exclude pawns behind or on sides of king. They can't legally capture
    pawns &= Attacks::FrontSquares[Us][pos.king_square(Us)];
    assert(Bitboard::bit_count(pawns) <= 3);  // There can only be two such pawns
    add_pinned_pawn_move<true, 7 * One, Us>(shift_bb<7 * One>(pawns) & CapSquares &
                                            Attacks::ADiagMaskEx[pos.king_square(Us)]);
    add_pinned_pawn_move<true, 9 * One, Us>(shift_bb<9 * One>(pawns) & CapSquares &
                                            Attacks::DiagMaskEx[pos.king_square(Us)]);
  }

  // Now, normal pawn pushes
  b1 = shift_bb<8 * One>(Pawns & NotPinned) & FreeSquares;
  b2 = shift_bb<8 * One>(b1 & (Rank3Mask << (Us * 3 * 8))) & FreeSquares;
  if (b1 &= Target) do {
    Square::Type to = lsb(b1);
    add_move(to - (8 * One), to);
  } while (b1 &= b1 - 1);

  // TODO : Try using a flag for double pawn pushes
  if (b2 &= Target) do {
    Square::Type to = lsb(b2);
    add_move(to - (16 * One), to);
  } while (b2 &= b2 - 1);

  // Now generate En-passant captures
  if ((pos.ep_square() != Square::NONE) && bit_set(Target, pos.ep_square() - (8 * One))) {
    uint64_t b = Pawns & Attacks::PawnAttacks[!Us][pos.ep_square()];
    if (b) do {
      Square::Type from = lsb(b);
      const uint64_t Occ =
        (pos.all_pieces() ^ sq_mask(from, pos.ep_square() - (8 * One))) | sq_mask(pos.ep_square());
      using TheirPcs = Piece::PieceOfColor < Color::Type(!Us) > ;
      if (!(Attacks::slider_attacks<Piece::ROOK>(pos.king_square(Us), Occ)
        & pos.pieces(TheirPcs::Rook, TheirPcs::Queen))
        && !(Attacks::slider_attacks<Piece::BISHOP>(pos.king_square(Us), Occ)
        & pos.pieces(TheirPcs::Bishop, TheirPcs::Queen)))
        add_move<Move::Flags::ENPASSANT>(from, pos.ep_square());
    } while (b &= b - 1);
  }
}

template <Piece::PieceType P>
void MoveGen::gen_slider_moves(uint64_t pieces, uint64_t target)
{
  static_assert((P == Piece::ROOK) || (P == Piece::BISHOP) || (P == Piece::QUEEN),
                "Piece must be a slider");
  uint64_t pc;

  // Generation for non pinned pieces
  if (pc = pieces & NotPinned) do {
    Square::Type from = Bitboard::lsb(pc);
    add_moves(from, Attacks::slider_attacks<P>(from, pos.all_pieces()) & target);
  } while (pc &= pc - 1);

  // Generation for pinned pieces
  if (pc = pieces & pos.pinned()) do {
    Square::Type from = Bitboard::lsb(pc);
    add_moves(from, Attacks::slider_attacks<P>(from, pos.all_pieces()) & target &
              Attacks::LineBetween[from][pos.king_square(pos.side_to_move())]);
  } while (pc &= pc - 1);
}

template <Color::Type Us, bool InCheck>
void MoveGen::gen_king_moves(uint64_t target)
{
  using namespace Attacks;
  uint64_t king_attacks = KingAttacks[pos.king_square(Us)] & target;

  if (InCheck) {
    // Skip known illegal moves
    uint64_t sliders = pos.checkers();
    sliders &= ~pos.pieces(Piece::make_piece(Piece::PAWN, ~Us), Piece::make_piece(Piece::KNIGHT, ~Us));
    if (sliders) do {
      Square::Type csq = Bitboard::lsb(sliders);
      king_attacks &=
        ~(Attacks::LineBetween[pos.king_square(Us)][csq] ^ Bitboard::sq_mask(csq));
    } while (sliders &= sliders - 1);
  }

  if (king_attacks) do {
    Square::Type to = Bitboard::lsb(king_attacks);
    if (pos.is_attacked<Us>(to))
      continue;  // Illegal move
    add_move(pos.king_square(Us), to);
  } while (king_attacks &= king_attacks - 1);

  if (InCheck) return;
  if (pos.can_castle_OO<Us>())
    add_move<Move::Flags::CASTLING>(Square::flip<Us>(Square::E1), Square::flip<Us>(Square::G1));
  if (pos.can_castle_OOO<Us>())
    add_move<Move::Flags::CASTLING>(Square::flip<Us>(Square::E1), Square::flip<Us>(Square::C1));
}

void MoveGen::gen_knight_moves(uint64_t target)
{
  // A pinned knight can't legally move
  uint64_t knights = pos.pieces(Piece::make_piece(Piece::KNIGHT, pos.side_to_move())) & NotPinned;
  if (knights) do {
    Square::Type from = Bitboard::lsb(knights);
    add_moves(from, Attacks::KnightAttacks[from] & target);
  } while (knights &= knights - 1);
}

std::string MoveGen::to_str() const
{
  std::ostringstream out;
  const Move::Type * mlist_ptr = mstack;
  while (*mlist_ptr != Move::Type::NONE) {
    out << Move::to_str(*mlist_ptr++);
    if (!((mlist_ptr - mstack) % 13)) out << std::endl;
    else if (mlist_ptr != end) out << ", ";
  }
  assert(mlist_ptr == end);
  out << "\nTotal Size: " << int(end - mstack);
  return out.str();
}