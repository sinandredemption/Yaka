#ifndef INC_POSITION_H_
#define INC_POSITION_H_
#include "yaka.h"
#include "bitboard.h"
#include "zobrist.h"
#include "attacks.h"
#include "score.h"
#include <string>

struct GameLine {
  Color::Type active_color;
  Castling::Right castle_rights;
  Square::Type ep_sq; // En passant squares
  int fifty_move;
  Piece::Type captured_piece;
  Square::Type king_sq[Color::COLOR_NB];
  key_t key;

  uint64_t checking_pieces;
  uint64_t pinned_pieces;
};

class Position {
  Piece::Type board[Square::SQ_NB];
  uint64_t piece_BB[Piece::PIECE_NB];
  uint64_t color_BB[Color::COLOR_NB];
  GameLine game_line;
  int game_ply;
public:
  Position() {};
  ~Position() {};
  void handle_error(const char * str);
  void parse_fen(const std::string &fen);
  std::string to_fen() const;
  std::string to_str() const;
  bool is_ok() const;
  bool is_ok(Move::Type m) const;
  Position flip();

  // Getters
  inline Square::Type ep_square() const;
  inline Castling::Right castling_rights() const;
  inline uint64_t checkers() const;
  inline Color::Type side_to_move() const;
  inline uint64_t pinned() const;
  inline int half_move() const;
  inline Square::Type king_square(Color::Type c) const;
  inline uint64_t pawns(Color::Type Us) const;
  inline Piece::Type piece(Square::Type sq) const;
  inline uint64_t pieces(Piece::Type pc) const;
  template <typename P, typename... Rest>
  inline uint64_t pieces(P pc, Rest... rest) const;
  inline uint64_t pieces(Color::Type cl) const;
  inline uint64_t all_pieces() const;
  template <Piece::PieceType Pt> inline uint64_t attacks_by(Square::Type sq) const;
  void make_move(Move::Type m, GameLine& gl);
  void make_null_move(GameLine& gl);
  void make_see_move(Color::Type us, Move::Type m, Piece::Type* cap);
  void unmake_move(Move::Type m, const GameLine& gl);
  void unmake_null_move(const GameLine& gl);
  void unmake_see_move(Color::Type us, Move::Type m, Piece::Type cap);
  template <Color::Type Us> inline bool is_attacked(Square::Type sq) const;
  template <Color::Type Us> inline uint64_t attackers_to(Square::Type sq) const;
  template <Color::Type Us> uint64_t pinned() const;
  template <Color::Type Us> inline bool can_castle_OO() const;
  template <Color::Type Us> inline bool can_castle_OOO() const;
  inline key_t hash() const;

  // Static Exchange evaluation for a move
  int see(Color::Type us, Move::Type m);
private:
  void reset();
  void update();

  // Manipulation of board
  template <bool UpdateGameLine = true>
  inline void set_piece(Piece::Type pc, Square::Type sq);
  template <bool UpdateGameLine = true>
  inline void move_piece(Piece::Type pc, Square::Type from, Square::Type to);
  template <bool UpdateGameLine = true>
  inline void remove_piece(Piece::Type pc, Square::Type sq);
};

inline Square::Type Position::ep_square() const
{
  return game_line.ep_sq;
}

inline Castling::Right Position::castling_rights() const
{
  return game_line.castle_rights;
}

inline uint64_t Position::checkers() const
{
  return game_line.checking_pieces;
}

inline Color::Type Position::side_to_move() const
{
  return game_line.active_color;
}

inline uint64_t Position::pinned() const
{
  return game_line.pinned_pieces;
}

inline int Position::half_move() const
{
  return game_line.fifty_move;
}

inline Square::Type Position::king_square(Color::Type c) const
{
  return game_line.king_sq[c];
}

inline uint64_t Position::pawns(Color::Type Us) const
{
  return piece_BB[Us == Color::WHITE ? Piece::WHITE_PAWN : Piece::BLACK_PAWN];
}

inline Piece::Type Position::piece(Square::Type sq) const
{
  return board[sq];
}

inline uint64_t Position::pieces(Piece::Type pc) const
{
  return piece_BB[pc];
}

inline uint64_t Position::pieces(Color::Type cl) const
{
  return color_BB[cl];
}

inline uint64_t Position::all_pieces() const
{
  return piece_BB[Piece::ALL_PIECES];
}

template <typename P, typename... Rest>
inline uint64_t Position::pieces(P pc, Rest... rest) const
{
  return pieces(pc) | pieces(rest...);
}

template <Piece::PieceType Pt>
inline uint64_t Position::attacks_by(Square::Type sq) const
{
  if (Pt == Piece::KNIGHT) return Attacks::KnightAttacks[sq];
  else return Attacks::slider_attacks<Pt>(sq, all_pieces());
}

inline key_t Position::hash() const
{
  return game_line.key;
}

template <bool UpdateGameLine>
inline void Position::set_piece(Piece::Type pc, Square::Type sq)
{
  assert(pc < Piece::ALL_PIECES);
  assert(sq < Square::SQ_NB);
  assert(board[sq] == Piece::NONE);
  board[sq] = pc;

  assert(!Bitboard::bit_set(piece_BB[pc], sq));
  assert(!Bitboard::bit_set(piece_BB[Piece::ALL_PIECES], sq));

  piece_BB[pc] |= Bitboard::sq_mask(sq);
  piece_BB[Piece::ALL_PIECES] |= Bitboard::sq_mask(sq);

  assert(!Bitboard::bit_set(color_BB[Piece::color_of(pc)], sq));
  color_BB[Piece::color_of(pc)] |= Bitboard::sq_mask(sq);

  if (UpdateGameLine)
    game_line.key ^= Zobrist::PieceHash[pc][sq];
}

template <bool UpdateGameLine>
inline void Position::move_piece(Piece::Type pc, Square::Type from, Square::Type to)
{
  assert(board[from] == pc);
  assert(board[to] == Piece::NONE);

  board[from] = Piece::NONE;
  board[to] = pc;
  const uint64_t FromToMask = Bitboard::sq_mask(from, to);

  assert(!Bitboard::bit_set(piece_BB[pc], to));
  piece_BB[pc] ^= FromToMask;

  assert(!Bitboard::bit_set(piece_BB[Piece::ALL_PIECES], to));
  piece_BB[Piece::ALL_PIECES] ^= FromToMask;

  assert(!Bitboard::bit_set(color_BB[Piece::color_of(pc)], to));
  color_BB[Piece::color_of(pc)] ^= FromToMask;

  if (UpdateGameLine)
    game_line.key ^= Zobrist::hash(pc, from, to);
}

template <bool UpdateGameLine>
inline void Position::remove_piece(Piece::Type pc, Square::Type sq)
{
  assert(board[sq] == pc);
  board[sq] = Piece::NONE;

  assert(Bitboard::bit_set(piece_BB[pc], sq));
  piece_BB[pc] ^= Bitboard::sq_mask(sq);

  assert(Bitboard::bit_set(piece_BB[Piece::ALL_PIECES], sq));
  piece_BB[Piece::ALL_PIECES] ^= Bitboard::sq_mask(sq);

  assert(Bitboard::bit_set(color_BB[Piece::color_of(pc)], sq));
  color_BB[Piece::color_of(pc)] ^= Bitboard::sq_mask(sq);
  if (UpdateGameLine)
    game_line.key ^= Zobrist::PieceHash[pc][sq];
}

template <Color::Type Us>
bool Position::is_attacked(Square::Type sq) const
{
  using namespace Attacks;
  using TheirPcs = Piece::PieceOfColor < Color::Type(!Us) > ;
  if (PawnAttacks[Us][sq] & pieces(TheirPcs::Pawn)) return true;
  if (KnightAttacks[sq] & pieces(TheirPcs::Knight)) return true;
  if (slider_attacks<Piece::BISHOP>(sq, all_pieces()) &
      pieces(TheirPcs::Bishop, TheirPcs::Queen)) return true;
  if (slider_attacks<Piece::ROOK>(sq, all_pieces()) &
      pieces(TheirPcs::Rook, TheirPcs::Queen)) return true;
  if (KingAttacks[sq] & pieces(TheirPcs::King)) return true;

  return false;
}

template <Color::Type Us>
inline bool Position::can_castle_OO() const
{
  if (!Castling::can_castle_OO<Us>(castling_rights())) return false;
  assert(!is_attacked<Us>(Square::flip<Us>(Square::E1)));
  assert(king_square(Us) == Square::flip<Us>(Square::E1));
  assert(Piece::piece_type(board[Square::flip<Us>(Square::H1)]) == Piece::ROOK);
  if (all_pieces() & (0x60ULL << (56 * Us))) return false;
  if (is_attacked<Us>(Square::flip<Us>(Square::F1))) return false;
  if (is_attacked<Us>(Square::flip<Us>(Square::G1))) return false;
  return true;
}

template <Color::Type Us>
inline bool Position::can_castle_OOO() const
{
  if (!Castling::can_castle_OOO<Us>(castling_rights())) return false;
  assert(!is_attacked<Us>(Square::flip<Us>(Square::E1)));
  assert(king_square(Us) == Square::flip<Us>(Square::E1));
  assert(Piece::piece_type(board[Square::flip<Us>(Square::A1)]) == Piece::ROOK);
  if (all_pieces() & (0xEULL << (56 * Us))) return false;
  if (is_attacked<Us>(Square::flip<Us>(Square::D1))) return false;
  if (is_attacked<Us>(Square::flip<Us>(Square::C1))) return false;
  return true;
}

// Returns all attackers to a given square that aren't of the given color
template <Color::Type Us>
uint64_t Position::attackers_to(Square::Type sq) const
{
  using namespace Attacks;
  using TheirPcs = Piece::PieceOfColor < Color::Type(!Us) > ;
  return (PawnAttacks[Us][sq] & pieces(TheirPcs::Pawn)) |
    (KnightAttacks[sq] & pieces(TheirPcs::Knight)) |
    (slider_attacks<Piece::BISHOP>(sq, all_pieces()) &
    pieces(TheirPcs::Bishop, TheirPcs::Queen)) |
    (slider_attacks<Piece::ROOK>(sq, all_pieces()) &
    pieces(TheirPcs::Rook, TheirPcs::Queen)) |
    (KingAttacks[sq] & Bitboard::sq_mask(game_line.king_sq[!Us]));
}

#endif
