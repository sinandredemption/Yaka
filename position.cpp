#include "position.h"
#include "misc.h"
#include <iostream>
#include <algorithm>
void Position::reset()
{
  std::memset(this, 0, sizeof(Position));
  game_line.active_color = Color::NONE;
  game_line.captured_piece = Piece::NONE;
  game_line.ep_sq = Square::NONE;
  for (Square::Type sq = Square::A1; sq <= Square::H8; ++sq)
    board[sq] = Piece::NONE;
}

void Position::handle_error(const char * str)
{
  puts(str);
  exit(EXIT_FAILURE);
}

void Position::update()
{
  using namespace Piece;
  game_line.king_sq[Color::WHITE] = Bitboard::lsb(pieces(Piece::WHITE_KING));
  game_line.king_sq[Color::BLACK] = Bitboard::lsb(pieces(Piece::BLACK_KING));

  if (side_to_move() == Color::WHITE) {
    game_line.checking_pieces = attackers_to<Color::WHITE>(game_line.king_sq[Color::WHITE]);
    game_line.pinned_pieces = pinned<Color::WHITE>();
  } else {
    game_line.checking_pieces = attackers_to<Color::BLACK>(game_line.king_sq[Color::BLACK]);
    game_line.pinned_pieces = pinned<Color::BLACK>();
  }
}

void Position::parse_fen(const std::string &fen_str)
{
  reset();
  std::vector<std::string> fen;
  Misc::split_string(fen_str, &fen);

  Square::Rank rk = Square::RANK_8;
  Square::File fl = Square::FILE_A;
  size_t i = 0, idx;
  while (i < fen[0].length()) {
    if (isdigit(fen[0][i]))
      fl = fl + int(fen[0][i] - '0');
    else if (fen[0][i] == '/')
      --rk, fl = Square::FILE_A;
    else if ((idx = Piece::PieceToChar.find(fen[0][i])) != std::string::npos)
      set_piece(Piece::Type(idx), Square::square_of(fl, rk)), ++fl;
    else handle_error("FEN PARSE ERROR: Invalid character found in fen string");
    ++i;
  }

  if (fen[1][0] == 'w')
    game_line.active_color = Color::WHITE;
  else if (fen[1][0] == 'b') {
    game_line.active_color = Color::BLACK;
    game_line.key ^= Zobrist::SideHash;
  } else handle_error("FEN PARSE ERROR: Invalid color");

  Castling::Right cr = Castling::NONE;
  for (i = 0; i < fen[2].length(); ++i) {
    switch (fen[2][i]) {
    case 'K': cr |= Castling::WHITE_OO; break;
    case 'Q': cr |= Castling::WHITE_OOO; break;
    case 'k': cr |= Castling::BLACK_OO; break;
    case 'q': cr |= Castling::BLACK_OOO; break;
    case '-': break;
    default: handle_error("FEN PARSE ERROR: Invalid Castling Flags");
    }
  }
  game_line.castle_rights = cr;
  game_line.key ^= Zobrist::CastlingHash[game_line.castle_rights];

  game_line.ep_sq = Square::NONE;
  if (fen[3][0] != '-') {
    game_line.ep_sq = Square::square_of(fen[3].c_str());
    game_line.key ^= Zobrist::EpHash[game_line.ep_sq];
  }

  if (fen.size() > 3)
    game_line.fifty_move = Misc::convert_to<int>(fen[4]);

  update();
  assert(is_ok());
}

std::string Position::to_fen() const
{
  std::stringstream ss;
  for (Square::Rank rk = Square::RANK_8; rk >= Square::RANK_1; --rk) {
    for (Square::File fl = Square::FILE_A; fl <= Square::FILE_H; ++fl) {

      int empty = 0;
      while (board[Square::square_of(fl, rk)] == Piece::NONE) {
        ++empty;
        if (++fl > Square::FILE_H) break;
      }

      if (empty) ss << empty;

      if (fl <= Square::FILE_H) ss << Piece::PieceToChar[board[Square::square_of(fl, rk)]];
    }
    if (rk > Square::RANK_1) ss << '/';
  }

  ss << (side_to_move() == Color::WHITE ? " w " : " b ");
  ss << Castling::to_str(castling_rights()) << ' ';
  ss << Square::to_str(ep_square()) << ' ';
  ss << game_line.fifty_move << ' ' << game_ply;

  return ss.str();
}

std::string Position::to_str() const
{
  std::ostringstream ss;
  for (Square::Rank r = Square::RANK_8; r >= Square::RANK_1; --r) {
    ss << "    +---+---+---+---+---+---+---+---+\n";
    ss.width(3);
    ss << r + 1 << " |";
    for (Square::File f = Square::FILE_A; f <= Square::FILE_H; ++f)
      ss << ' ' << Piece::PieceToChar[board[Square::square_of(f, r)]] << " |";
    ss << '\n';
  }
  ss << "    +---+---+---+---+---+---+---+---+\n";
  ss << "      a   b   c   d   e   f   g   h  \n";

  ss << "Fen: " << to_fen() << std::endl;
  ss << "Key: " << Misc::hash << game_line.key << Misc::unhash;
  uint64_t b = pinned();
  if (b) ss << "\nPinned Pieces: ";
  while (b) {
    ss << Square::to_str(Bitboard::pop_1st_bit(&b));
    if (b) ss << ", ";
  }
  b = checkers();
  if (b) ss << "\nCheckers: ";
  while (b) {
    ss << Square::to_str(Bitboard::pop_1st_bit(&b));
    if (b) ss << ", ";
  }
  return ss.str();
}

template <Color::Type Us>
uint64_t Position::pinned() const
{
  using TheirPcs = Piece::PieceOfColor < Color::Type(!Us) > ;
  uint64_t pinner = pieces(TheirPcs::Queen, TheirPcs::Rook) &
    *Attacks::RAttacks[king_square(Us)];
  pinner |= pieces(TheirPcs::Queen, TheirPcs::Bishop) &
    *Attacks::BAttacks[king_square(Us)];

  uint64_t pinned_pcs = 0, b;

  if (pinner) do {
    b = Attacks::SqBetween[king_square(Us)][Bitboard::lsb(pinner)] & all_pieces();

    if (!Bitboard::more_than_one(b))
      pinned_pcs |= b & pieces(Us);
  } while (pinner &= pinner - 1);

  return pinned_pcs;
}

void Position::make_move(Move::Type m, GameLine& gl)
{
  assert(is_ok(m));
  // Directly copy the current game line into new_line
  std::memcpy(&gl, &game_line, sizeof(GameLine));

  const Square::Type From = Move::from_sq(m);
  const Square::Type To = Move::to_sq(m);
  const Move::Flags Flag = Move::flags(m);
  const Piece::Type Pc = board[From];
  const Piece::PieceType Pt = Piece::piece_type(Pc);
  const Piece::Type Capture = board[To];
  const Color::Type Us = Piece::color_of(Pc);

  game_line.captured_piece = board[To];
  assert(side_to_move() == Us);
  game_line.active_color = ~Us;
  game_line.key ^= Zobrist::SideHash;
  ++game_line.fifty_move;
  ++game_ply;

  if (game_line.ep_sq != Square::NONE)
    game_line.key ^= Zobrist::EpHash[game_line.ep_sq];

  game_line.ep_sq = Square::NONE; // reset enpassant capture square

  /*
  ** Now we'll 'apply' the move. Besides simply moving the piece, things to
  ** take into consideration when making a move:
  **
  ** 1. If the move is a normal capture (not en-passant capture), then:
  **    (i) remove the captured piece (ii) update the castling rights (in
  **    case captured piece is a rook) (iii) reset the fifty move clock
  **
  ** 2. If the moving piece is a pawn then reset the half move clock, then:
  **    (i) handle en-passant capture (ii) handle promotions
  **    (iii) set the en-passant square if it's a double pawn push.
  **
  ** 3. If the moving piece is king, then:
  **    (i) restrict castling (ii) handle castling
  **    We will also update the king squares in case of king moves.
  **
  ** 4. If the moving piece is rook, update the castling rights.
  */

  // Handle Castling rights. No matter which piece moves from squares
  // a1, h1, a8, h8, e1, and e8, we remove castling rights associated
  // with that square
  game_line.key ^= Zobrist::CastlingHash[game_line.castle_rights];
  game_line.castle_rights &= Castling::CastleRights[From];
  game_line.castle_rights &= Castling::CastleRights[To];
  game_line.key ^= Zobrist::CastlingHash[game_line.castle_rights];

  // 1. Handle captures
  if (Capture != Piece::NONE) // 1. (i) Remove the captured piece
    remove_piece(Capture, To), game_line.fifty_move = 0;
  // En-Passant captures are handled later

  // Move the piece normally, except in case of promotion
  if (Flag != Move::Flags::PROMOTION)
    move_piece(Pc, From, To);
  else {
    // 2. (ii) Handle Promotions
    remove_piece(Piece::make_piece(Piece::PAWN, Us), From);
    set_piece(Piece::make_piece(Move::promotion_pc(m), Us), To);
    game_line.fifty_move = 0;
    goto end;
  }

  // 2. Handle Pawn moves
  if (Pt == Piece::PAWN) {
    game_line.fifty_move = 0;
    // 2. (i) Handle en-passant
    if (Flag == Move::Flags::ENPASSANT)
      remove_piece(Piece::make_piece(Piece::PAWN, ~Us), Square::south(To, Us));
    else if ((From ^ To) == 16) { // 2. (iii) Set en-passant square
      game_line.ep_sq = Square::Type((From + To) >> 1);
      game_line.key ^= Zobrist::EpHash[ep_square()];
    }
  }

  // 3. Handle King moves
  else if (Pt == Piece::KING) {
    // 3. (i) Handle Castling
    if (Flag == Move::Flags::CASTLING) {
      // We already have moved the king, now we need to move the rook
      if (Castling::is_kingside_castle(To))
        move_piece(Piece::make_piece(Piece::ROOK, Us), From + 3, From + 1); // King side castling
      else
        move_piece(Piece::make_piece(Piece::ROOK, Us), From - 4, From - 1);  // Queen side castling
    }
    // Update the king squares
    game_line.king_sq[Us] = To;
  }

end:
  if (Us == Color::WHITE) {
    game_line.checking_pieces = attackers_to<Color::BLACK>(game_line.king_sq[Color::BLACK]);
    game_line.pinned_pieces = pinned<Color::BLACK>();
  } else {
    game_line.checking_pieces = attackers_to<Color::WHITE>(game_line.king_sq[Color::WHITE]);
    game_line.pinned_pieces = pinned<Color::WHITE>();
  }
  assert(is_ok());
}

void Position::make_null_move(GameLine& gl) {
  std::memcpy(&gl, &game_line, sizeof(GameLine));

  game_line.key ^= Zobrist::SideHash;
  ++game_line.fifty_move;

  if (game_line.ep_sq != Square::NONE) {
    game_line.key ^= Zobrist::EpHash[game_line.ep_sq];
    game_line.ep_sq = Square::NONE;
  }

  if (game_line.active_color == Color::WHITE) {
    game_line.checking_pieces = attackers_to<Color::BLACK>(game_line.king_sq[Color::BLACK]);
    game_line.pinned_pieces = pinned<Color::BLACK>();
  }
  else {
    game_line.checking_pieces = attackers_to<Color::WHITE>(game_line.king_sq[Color::WHITE]);
    game_line.pinned_pieces = pinned<Color::WHITE>();
  }

  game_line.active_color = ~game_line.active_color;

  assert(is_ok());
}

void Position::make_see_move(Color::Type us, Move::Type m, Piece::Type* cap)
{
  assert(!Move::is_move(m, Move::Flags::CASTLING));
  assert(!Move::is_move(m, Move::Flags::PROMOTION));
  
  Square::Type from_sq = Move::from_sq(m), to_sq = Move::to_sq(m);
  *cap = board[to_sq];

  if (Move::is_move(m, Move::Flags::ENPASSANT)) {
    assert(Piece::piece_type(board[to_sq - Square::south(to_sq, us)]) == Piece::PAWN);
    remove_piece<false>(Piece::make_piece(Piece::PAWN, us), Square::south(to_sq, us));
  }
  else {
    assert(*cap != Piece::NONE);
    remove_piece<false>(board[to_sq], to_sq);
  }

  move_piece<false>(board[from_sq], from_sq, to_sq);
}

void Position::unmake_move(Move::Type m, const GameLine& gl)
{
  const Square::Type To = Move::to_sq(m);
  const Square::Type From = Move::from_sq(m);
  const Piece::Type Pc = board[To];
  const Color::Type Us = Piece::color_of(Pc);
  assert(side_to_move() != Us);
  const Move::Flags Flag = Move::flags(m);

  assert(board[From] == Piece::NONE);

  if (Flag != Move::Flags::PROMOTION)
    move_piece<false>(Pc, To, From);
  else {  // Promotion
    remove_piece<false>(Pc, To); // Remove this piece
    set_piece<false>(Piece::make_piece(Piece::PAWN, Us), From);
  }

  // Handle captures
  if (game_line.captured_piece != Piece::NONE)
    set_piece<false>(game_line.captured_piece, To);

  // Handle en-passant
  if (Flag == Move::Flags::ENPASSANT)
    set_piece<false>(Piece::make_piece(Piece::PAWN, ~Us), Square::south(To, Us));

  // Handle castling
  if (Flag == Move::Flags::CASTLING) {
    if (Castling::is_kingside_castle(To))
      move_piece<false>(Piece::make_piece(Piece::ROOK, Us), From + 1, From + 3);
    else
      move_piece<false>(Piece::make_piece(Piece::ROOK, Us), From - 1, From - 4);
  }

  // Directly copy the game line
  std::memcpy(&game_line, &gl, sizeof(GameLine));
  --game_ply;
  assert(is_ok());
}

void Position::unmake_null_move(const GameLine& gl) {
  assert(!checkers());
  std::memcpy(&game_line, &gl, sizeof(GameLine));
  assert(is_ok());
}

inline void Position::unmake_see_move(Color::Type us, Move::Type m, Piece::Type cap)
{
  Square::Type from_sq = Move::from_sq(m), to_sq = Move::to_sq(m);
  assert(board[from_sq] == Piece::NONE);

  move_piece<false>(board[to_sq], to_sq, from_sq);

  if (cap != Piece::NONE)
    set_piece<false>(cap, to_sq);

  if (Move::is_move(m, Move::Flags::ENPASSANT))
    set_piece<false>(Piece::make_piece(Piece::PAWN, ~us), Square::south(to_sq, us));
}

int Position::see(Color::Type us, Move::Type m)
{
  Color::Type stm = ~us;
  int gain[32], d = 0;
  Square::Type to_sq = Move::to_sq(m), from_sq = Move::from_sq(m);

  Piece::Type captured;
  make_see_move(us, m, &captured);

  uint64_t occupied = all_pieces() ^ Bitboard::sq_mask(from_sq), attack;

  if (Move::is_move(m, Move::Flags::ENPASSANT)) {
    gain[0] = Scores::PawnValue.mg;
    occupied ^= Bitboard::sq_mask(Square::south(to_sq, us));
  }
  else {
    Piece::PieceType pt = Piece::piece_type(board[to_sq]);
    if (pt == Piece::KING)
      return Score::MATE_SCORE;

    gain[0] = Scores::PieceVal[pt].mg;
  }

  while (true) {
    d++;
    int best = Score::MATE_SCORE;
    Piece::Type p = Piece::make_piece(Piece::PAWN, stm);

    attack = Attacks::PawnAttacks[~stm][to_sq] & piece_BB[p] & occupied;
    if (attack)
      best = Scores::PawnValue.mg;
    else {
      p = Piece::make_piece(Piece::KNIGHT, stm);
      attack = Attacks::KnightAttacks[to_sq] & piece_BB[p] & occupied;

      if (attack)
        best = Scores::KnightValue.mg;
      else {
        p = Piece::make_piece(Piece::BISHOP, stm);
        attack = Attacks::slider_attacks<Piece::BISHOP>(to_sq, occupied);
        attack &= piece_BB[p];

        if (attack)
          best = Scores::BishopValue.mg;
        else {
          p = Piece::make_piece(Piece::ROOK, stm);
          attack = Attacks::slider_attacks<Piece::ROOK>(to_sq, occupied);
          attack &= piece_BB[p];

          if (attack)
            best = Scores::RookValue.mg;
          else {
            p = Piece::make_piece(Piece::QUEEN, stm);
            attack = Attacks::slider_attacks<Piece::QUEEN>(to_sq, occupied);
            attack &= piece_BB[p];

            if (attack)
              best = Scores::QueenValue.mg;

            else {
              p = Piece::make_piece(Piece::KING, stm);
              attack = Attacks::KingAttacks[to_sq] & piece_BB[p];

              if (attack)
                best = Score::MATE_SCORE;
              else
                break;
            }
          }
        }
      }
    }

    gain[d] = best - gain[d - 1];
    if (best = Score::MATE_SCORE)
      break;

    occupied &= ~(attack & -attack);
    stm = ~stm;
  }

  unmake_see_move(us, m, captured);

  while (--d)
    gain[d - 1] = -std::max(-gain[d - 1], gain[d]);

  return gain[0];
}

bool Position::is_ok() const
{
  using namespace std;
  Square::Type sq;
  uint64_t b;
  key_t key_ = 0;
  for (Piece::Type pc = Piece::WHITE_PAWN; pc <= Piece::BLACK_KING; ++pc) {
    b = piece_BB[pc];
    Color::Type cl = Piece::color_of(pc);
    if ((b | color_BB[cl]) != color_BB[cl]) {
      cerr << "Invalid Piece Bitboards for Piece: " << Piece::to_str(pc) << endl;
      return false;
    }
    if ((b | piece_BB[Piece::ALL_PIECES]) != piece_BB[Piece::ALL_PIECES]) {
      cerr << "Invalid Piece Bitboards for Piece: " << Piece::to_str(pc) << endl;
      return false;
    }

    while (b) {
      sq = Bitboard::pop_1st_bit(&b);
      key_ ^= Zobrist::PieceHash[pc][sq];
      if (board[sq] != pc) {
        cerr << "board[" << Square::to_str(sq) << "] = " << Piece::to_str(board[sq]) << endl;
        cerr << "Bitboards claim it to be: " << Piece::to_str(pc) << endl;
        return false;
      }
    }
  }
  if (Bitboard::more_than_one(piece_BB[Piece::WHITE_KING])) {
    cerr << "White has more than one king" << endl;
    return false;
  }
  if (Bitboard::more_than_one(piece_BB[Piece::BLACK_KING])) {
    cerr << "Black has more than one king" << endl;
    return false;
  }
  if (Bitboard::lsb(piece_BB[Piece::WHITE_KING]) != game_line.king_sq[Color::WHITE]) {
    cerr << "Incorrect White king square: " << Square::to_str(game_line.king_sq[Color::WHITE]) << endl;
    cerr << "Bitboards claim it to be: " << Square::to_str(Bitboard::lsb(piece_BB[Piece::WHITE_KING])) << endl;
    return false;
  }
  if (Bitboard::lsb(piece_BB[Piece::BLACK_KING]) != game_line.king_sq[Color::BLACK]) {
    cerr << "Incorrect Black king square: " << Square::to_str(game_line.king_sq[Color::BLACK]) << endl;
    cerr << "Bitboards claim it to be: " << Square::to_str(Bitboard::lsb(piece_BB[Piece::BLACK_KING])) << endl;
    return false;
  }
  if (game_line.castle_rights >= Castling::CASTLING_RIGHT_NB) {
    cerr << "Incorrect Castling Rights" << endl;
    return false;
  }
  if (game_line.ep_sq >= Square::SQ_NB) {
    cerr << "Incorrect En Passant Square" << endl;
    return false;
  }
  key_ ^= Zobrist::CastlingHash[game_line.castle_rights];
  if (side_to_move() == Color::BLACK)
    key_ ^= Zobrist::SideHash;
  if (ep_square() != Square::NONE)
    key_ ^= Zobrist::EpHash[ep_square()];
  if (key_ != game_line.key) {
    cerr << "Incorrect Key: " << Misc::hash << game_line.key << endl;
    cerr << "Should be:     " << Misc::hash << key_ << Misc::unhash << endl;
    return false;
  }
  if (pinned() != (side_to_move() == Color::WHITE ? pinned<Color::WHITE>() : pinned<Color::BLACK>())) {
    cerr << "Pinned pieces not updated correctly" << endl;
    cerr << "Currently are:\n" << Bitboard::to_str(pinned()) << "but should be:\n"
      << Bitboard::to_str(side_to_move() == Color::WHITE ? pinned<Color::WHITE>() : pinned<Color::BLACK>());
    cerr << " position: " << to_fen() << endl;
    return false;
  }
  return true;
}

bool Position::is_ok(Move::Type m) const
{
  const Color::Type Us = side_to_move();
  const Square::Type From = Move::from_sq(m);
  const Square::Type To = Move::to_sq(m);
  const Move::Flags Flag = Move::flags(m);
  const Piece::Type Pc = board[From];
  const Piece::PieceType Pt = Piece::piece_type(Pc);
  const Piece::Type Capture = board[To];
  const Color::Type Them = ~Us;

  using namespace std;
  if (From == To) {
    cerr << "From and To squares of the move: " << Move::to_str(m) << " are equal.";
    return false;
  }
  if (Piece::color_of(Pc) != Us) {
    cerr << "Color of moving piece isn't the color to move.";
    return false;
  }
  if (Capture != Piece::NONE) {
    if (Piece::color_of(Capture) != Them) {
      cerr << "Color of captured piece should be the opposite color.";
      return false;
    }
    if (Piece::piece_type(Capture) == Piece::KING) {
      cerr << "King can not be captured!";
      return false;
    }
    if (Flag == Move::Flags::CASTLING) {
      cerr << "Castling shouldn't be involved in a capture.";
      return false;
    }
  }
  if (Pt == Piece::PAWN) {
    // En-passant
    if (Flag == Move::Flags::ENPASSANT) {
      if (Pt != Piece::PAWN) {
        cerr << "Moving piece must be a pawn during an En-passant capture";
        return false;
      }
      if (board[To] != Piece::NONE) {
        cerr << "Pawn must move to an empty square after an En-passant capture";
        return false;
      }
      if (board[To + ((Us << 4) - 8)] != Piece::make_piece(Piece::PAWN, Them)) {
        cerr << "Captured piece must be pawn in an En-Passant capture.";
        return false;
      }
    }
  }
  return true;
}

Position Position::flip()
{
  Position out;
  out.reset();

  for (Square::Type sq = Square::A1; sq < Square::SQ_NB; ++sq)
    if (board[sq] != Piece::NONE)
      out.set_piece(~board[sq], Square::flip(sq));

  out.game_line.active_color = ~side_to_move();
  if (out.game_line.active_color == Color::BLACK)
    out.game_line.key ^= Zobrist::SideHash;

  if (castling_rights() & Castling::WHITE_OO)
    out.game_line.castle_rights |= Castling::BLACK_OO;

  if (castling_rights() & Castling::WHITE_OOO)
    out.game_line.castle_rights |= Castling::BLACK_OOO;

  if (castling_rights() & Castling::BLACK_OO)
    out.game_line.castle_rights |= Castling::WHITE_OO;

  if (castling_rights() & Castling::BLACK_OOO)
    out.game_line.castle_rights |= Castling::WHITE_OOO;
  out.game_line.key ^= Zobrist::CastlingHash[out.game_line.castle_rights];

  if (ep_square() != Square::NONE) {
    out.game_line.ep_sq = Square::flip(ep_square());
      out.game_line.key ^= Zobrist::EpHash[Square::flip(ep_square())];
  }

  out.update();
  assert(out.is_ok());
  return out;
}
