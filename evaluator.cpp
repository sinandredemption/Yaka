#include "evaluator.h"
#include "position.h"
#include "attacks.h"
#include <iostream>
#include <iomanip>
#include <sstream>

void Evaluator::init(const Position &pos_)
{
  std::memset(this, 0, sizeof(*this));
  pos = &pos_;

  attacks_by[Piece::WHITE_PAWN] = Attacks::pawn_attacks<Color::WHITE>(pos->pawns(Color::WHITE));
  attacks_by[Piece::BLACK_PAWN] = Attacks::pawn_attacks<Color::BLACK>(pos->pawns(Color::BLACK));

  passed_pawns[Color::WHITE] = passed_pawns[Color::BLACK] = 0;

  mobility_area[Color::WHITE] = ~pos->pieces(Color::WHITE);
  mobility_area[Color::WHITE] &= ~attacks_by[Piece::BLACK_PAWN];
  mobility_area[Color::BLACK] = ~pos->pieces(Color::BLACK);
  mobility_area[Color::BLACK] &= ~attacks_by[Piece::WHITE_PAWN];

  king_zone[Color::WHITE] = Attacks::KingAttacks[pos->king_square(Color::WHITE)];
  king_zone[Color::WHITE] |= king_zone[Color::WHITE] << 8;
  king_zone[Color::BLACK] = Attacks::KingAttacks[pos->king_square(Color::BLACK)];
  king_zone[Color::BLACK] |= king_zone[Color::BLACK] >> 8;

}

Evaluator::Evaluator(const Position& pos_)
{
  init(pos_);
}

template <Color::Type Us>
Score Evaluator::eval_pawns()
{
  using namespace Bitboard;
  const Color::Type Them = Color::Type(!Us);
  const int One = Us == Color::WHITE ? 1 : -1;
  Score s(0, 0);
  uint64_t pawns = pos->pawns(Us);

  s -= Scores::BackwardPawnPenalty *
    sparse_bit_count(pos->pawns(Us) & full_fill(attacks_by[Piece::make_piece(Piece::PAWN, ~Us)] &
      ~forward_fill<Us>(attacks_by[Piece::make_piece(Piece::PAWN, Us)])));

  bool doubled, isolated, connected, weak, passer;
  if (pawns) do {
    Square::Type sq = Bitboard::lsb(pawns);
    Square::File fl = Square::file_of(sq);
    Square::Rank rk = Square::rank_of(sq);
    const uint64_t PrevRank = Attacks::RankMaskEx[sq - (8 * One)];

    s += Scores::PcSqTab[Piece::PieceOfColor<Us>::Pawn][sq];

    // Flag the pawns
    doubled = (pos->pawns(Us) & Attacks::SqBetween[sq][(56 | fl) ^ (56 * Us)]) != 0;
    isolated = (pos->pawns(Us) & Attacks::NeighbouringFiles[fl]) == 0;
    connected = (pos->pawns(Us) & (Attacks::PawnAttacks[Them][sq] | Attacks::RankMaskEx[sq])) != 0;
    weak = ((pos->pawns(Us) & PrevRank) == 0) && (rk != (Square::RANK_2 ^ (7 * Us)));
    passer = (pos->pawns(Them) & Attacks::PassedPawnMask[Us][sq]) == 0;

    // Evaluate the pawns
    if (doubled)
      s -= Scores::DoubledPawnPenalty[fl];
    if (isolated)
      s -= Scores::IsolatedPawnPenalty[fl];
    if (connected)
      s += Scores::ConnectedPawnBonus[rk ^ (Us * 7)];
    if (weak && !isolated)
      s -= Scores::WeakPawnPenalty;

    if (passer && !doubled)
      passed_pawns[Us] |= sq_mask(sq);
  } while (pawns &= pawns - 1);

  return s;
}

template <Color::Type Us, Piece::PieceType Pt>
Score Evaluator::eval_pieces()
{
  const Piece::Type Pc = Piece::make_piece(Pt, Us);
  const Color::Type Them = ~Us;
  const int PtIndex = int(Pt) - 1;
  uint64_t bb = pos->pieces(Pc);
  Score s(0, 0);

  if (bb) do {
    Square::Type sq = Bitboard::lsb(bb);
    s += Scores::PcSqTab[Pc][sq];
    non_pawn_material += Scores::PieceVal[Pt].mg;

    uint64_t atk = pos->attacks_by<Pt>(sq);
    attacks_by[Pc] |= atk;
    atk &= mobility_area[Us];
    s += Scores::MobilityBonus[PtIndex][Bitboard::bit_count(atk)];

    // Outposts
    if ((Pt == Piece::KNIGHT) && !(Attacks::PawnAttackSpan[Us][sq] & pos->pawns(Them)))
      s += Scores::OutpostBonus[PtIndex][sq ^ (56 * Them)];
    if ((Pt == Piece::BISHOP) && !(Attacks::PawnAttackSpan[Us][sq] & pos->pawns(Them)))
      s += Scores::OutpostBonus[PtIndex][sq ^ (56 * Them)];

    // Bonus for a rook being on open or semi-open file
    if ((Pt == Piece::ROOK) && !(pos->pawns(Us) & Attacks::FileMaskEx[sq]))
      s += pos->pawns(Them) & Attacks::FileMaskEx[sq] ? Scores::RookOnSemiOpenFile : Scores::RookOnOpenFile;
  } while (bb &= bb - 1);

  // Bonus for Bishop pair
  if ((Pt == Piece::BISHOP) && Bitboard::more_than_one(pos->pieces(Pc))) s += Scores::BishopPairBonus;

  return s;
}

template <Color::Type Us>
Score Evaluator::eval_king_safety()
{
  using namespace Bitboard;
  using OurPieces = Piece::PieceOfColor < Us >;
  using TheirPieces = Piece::PieceOfColor < Color::Type(!Us) >;

  Score s(0, 0);
  const int One = Us == Color::WHITE ? 1 : -1;

  Square::Rank rk = Square::rank_of(pos->king_square(Us));
  Square::File fl = Square::file_of(pos->king_square(Us));

  // Evaluate Pawn Shields
  const uint64_t ShelterMask = FileMask[fl] | Attacks::NeighbouringFiles[fl];
  uint64_t OurPawns = pos->pawns(Us) & ShelterMask, EnemyPawns = pos->pawns(~Us) & ShelterMask;
  int shelter = 0;

  if ((rk ^ (7 * Us)) <= Square::RANK_4) {
    shelter += bit_count(OurPawns & RankMask[rk + (1 * One)]) * 192;
    shelter += bit_count(OurPawns & RankMask[rk + (2 * One)]) * 96;
    shelter += bit_count(OurPawns & RankMask[rk + (3 * One)]) * 48;

    shelter -= bit_count(EnemyPawns & RankMask[rk + (1 * One)]) * 256;
    shelter -= bit_count(EnemyPawns & RankMask[rk + (2 * One)]) * 128;
    shelter -= bit_count(EnemyPawns & RankMask[rk + (3 * One)]) * 64;

    s += Score(shelter, 0);
  }

  // Evaluate King Zone attacks
  int king_attacks = 0;
  uint64_t defended = attacks_by[OurPieces::Pawn] |
    attacks_by[OurPieces::Knight] | attacks_by[OurPieces::Bishop], attacked, defended_;
  all_attacks[Us] = defended;

  // TODO: Use function call
#define COUNT_KING_ATTACKS(pc, weight)                           \
  attacked = attacks_by[pc] & king_zone[Us];                     \
  defended_ = defended;                                          \
  defended &= ~attacked;                                         \
  king_attacks += Bitboard::sparse_bit_count(attacked) * weight; \
  king_attacks -= (Bitboard::sparse_bit_count(attacked & defended_) * weight) / 2;

  COUNT_KING_ATTACKS(TheirPieces::Knight, 3);
  COUNT_KING_ATTACKS(TheirPieces::Bishop, 3);

  defended |= attacks_by[OurPieces::Rook] | attacks_by[OurPieces::Queen];
  all_attacks[Us] |= defended;

  COUNT_KING_ATTACKS(TheirPieces::Rook, 5);
  COUNT_KING_ATTACKS(TheirPieces::Queen, 7);
#undef COUNT_KING_ATTACKS

  s.mg -= Scores::SafetyTable[king_attacks];
  return s;
}

template <Color::Type Us>
Score Evaluator::eval_threats()
{
  using OurPieces = Piece::PieceOfColor < Us >;
  using TheirPieces = Piece::PieceOfColor < Color::Type(!Us) >;
  Score s(0, 0);

  const uint64_t defended = pos->pieces(~Us) & attacks_by[TheirPieces::Pawn];
  const uint64_t undefended = pos->pieces(~Us) & ~attacks_by[TheirPieces::Pawn];
  uint64_t hanging = undefended &
    ~(attacks_by[TheirPieces::Knight] | attacks_by[TheirPieces::Bishop]
      | attacks_by[TheirPieces::Rook] | attacks_by[TheirPieces::Queen])
    & (attacks_by[OurPieces::Knight] | attacks_by[OurPieces::Bishop]
      | attacks_by[OurPieces::Rook] | attacks_by[OurPieces::Queen]);

  if (defended) {
    score_threat<OurPieces::Knight, true>(defended, s);
    score_threat<OurPieces::Bishop, true>(defended, s);
    score_threat<OurPieces::Rook, true>(defended, s);
    score_threat<OurPieces::Queen, true>(defended, s);
  }

  if (undefended) {
    score_threat<OurPieces::Knight, false>(undefended, s);
    score_threat<OurPieces::Bishop, false>(undefended, s);
    score_threat<OurPieces::Rook, false>(undefended, s);
    score_threat<OurPieces::Queen, false>(undefended, s);
  }

  s -= Scores::HangingPenalty * Bitboard::sparse_bit_count(hanging);
  return s;
}

template <Color::Type Us>
Score Evaluator::eval_passed_pawns()
{
  const Color::Type Them = Color::Type(!Us);
  const int One = Us == Color::WHITE ? 1 : -1;
  uint64_t b;
  Score s(0, 0);

  if (b = passed_pawns[Us]) do {
    Square::Type sq = Bitboard::lsb(b);
    Square::Rank rk = Square::Rank(Square::rank_of(sq) ^ (7 * Us));
    s += Scores::PassedPawnBonus[rk];

    // Bonus if supported
    if (Attacks::PawnAttacks[Them][sq] & pos->pawns(Us))
      s += Scores::PassedPawnBonus[rk];

    // Penalize if there are pieces on way to promotion
    uint64_t path = Attacks::FrontSquares[Us][sq] & Attacks::FileMaskEx[sq];
    s.eg -= 2 * Bitboard::sparse_bit_count(path & pos->pieces(Us));
    s.eg -= 4 * Bitboard::sparse_bit_count(path & pos->pieces(Them));

    // Bonus if defended
    if ((all_attacks[Us] | Attacks::KingAttacks[sq]) & Bitboard::sq_mask(sq))
      s += Scores::SupportedPasserBonus;

    // Penalty if attacked by opposing king
    if (Attacks::KingAttacks[pos->king_square(Them)] & Bitboard::sq_mask(sq))
      s -= Scores::SupportedPasserBonus * 4;

    // Penalty if surrounded by opposing king
    if (Attacks::KingAttacks[pos->king_square(Them)] & Attacks::KingAttacks[sq])
      s -= Scores::SupportedPasserBonus * 2;

    Square::Type push = sq + (8 * One);
    if (pos->piece(push) == Piece::NONE) {

      s += Scores::PasserNotBlocked * rk * rk * (rk > Square::RANK_4 ? rk : 1);

      // Bonus if push square is not attacked
      if (!(all_attacks[Them] & Bitboard::sq_mask(push)))
        s += Scores::PasserCanAdvance * rk;

      // Bonus if path to queen is properly defended
      if (!((all_attacks[Them] & ~all_attacks[Us]) & path))
        s += Scores::PasserPathClear * rk * rk * (rk > Square::RANK_4 ? rk : 1);

    }
    else if (Piece::color_of(pos->piece(push)) == Us) // If own piece on blocking square
      s += Scores::PasserBlocked * rk * rk * (rk > Square::RANK_4 ? rk : 1);
  } while (b &= b - 1);

  return s;
}

#define PRINT(x) std::setw(6) << to_cp(x)

void Evaluator::to_str(std::stringstream& ss, Score s1, Score s2, std::string entity)
{
  using namespace std;
  entity += ": ";
  ss << setw(16) << entity
    << PRINT(s1.mg) << " " << PRINT(s1.eg) << " | "
    << PRINT(s2.mg) << " " << PRINT(s2.eg) << " | "
    << PRINT(s1.mg - s2.mg) << " " << PRINT(s1.eg - s2.eg);
  ss << endl;
}

std::string Evaluator::to_str()
{
  Score s(0, 0), s1, s2;
  std::stringstream ss;
  ss << std::showpoint << std::noshowpos << std::fixed << std::setprecision(2);

  ss << "        Entity      White     |     Black     |     Total\n" << LongLine << std::endl;
  s1 = eval_pawns<Color::WHITE>();
  s2 = eval_pawns<Color::BLACK>();
  s += s1 - s2;
  to_str(ss, s1, s2, "Pawns");

  s1 = eval_pieces<Color::WHITE, Piece::KNIGHT>();
  s2 = eval_pieces<Color::BLACK, Piece::KNIGHT>();
  s += s1 - s2;
  to_str(ss, s1, s2, "Knights");

  s1 = eval_pieces<Color::WHITE, Piece::BISHOP>();
  s2 = eval_pieces<Color::BLACK, Piece::BISHOP>();
  s += s1 - s2;
  to_str(ss, s1, s2, "Bishops");

  s1 = eval_pieces<Color::WHITE, Piece::ROOK>();
  s2 = eval_pieces<Color::BLACK, Piece::ROOK>();
  s += s1 - s2;
  to_str(ss, s1, s2, "Rooks");

  s1 = eval_pieces<Color::WHITE, Piece::QUEEN>();
  s2 = eval_pieces<Color::BLACK, Piece::QUEEN>();
  s += s1 - s2;
  to_str(ss, s1, s2, "Queens");

  s1 = eval_king_safety<Color::WHITE>();
  s2 = eval_king_safety<Color::BLACK>();
  s += s1 - s2;
  to_str(ss, s1, s2, "King Safety");

  s1 = eval_threats<Color::WHITE>();
  s2 = eval_threats<Color::BLACK>();
  s += s1 - s2;
  to_str(ss, s1, s2, "Threats");

  s1 = eval_passed_pawns<Color::WHITE>();
  s2 = eval_passed_pawns<Color::BLACK>();
  s += s1 - s2;
  to_str(ss, s1, s2, "Passed Pawns");

  ss << LongLine << "\nTotal: ";
  ss << PRINT(s.mg) << " " << PRINT(s.eg);
  ss << std::endl;

  ss << "Non Pawn Material: " << PRINT(non_pawn_material) << std::endl;
  ss << "Final Eval: " << PRINT(interpolate_score(s, pos->side_to_move())) << std::endl;
  return ss.str();
}

int Evaluator::eval()
{
  init(*pos);
  Score s(0, 0);

  s += eval_pawns<Color::WHITE>() - eval_pawns<Color::BLACK>();
  s += eval_pieces<Color::WHITE, Piece::KNIGHT>() - eval_pieces<Color::BLACK, Piece::KNIGHT>();
  s += eval_pieces<Color::WHITE, Piece::BISHOP>() - eval_pieces<Color::BLACK, Piece::BISHOP>();
  s += eval_pieces<Color::WHITE, Piece::ROOK>() - eval_pieces<Color::BLACK, Piece::ROOK>();
  s += eval_pieces<Color::WHITE, Piece::QUEEN>() - eval_pieces<Color::BLACK, Piece::QUEEN>();
  s += eval_king_safety<Color::WHITE>() - eval_king_safety<Color::BLACK>();
  s += eval_threats<Color::WHITE>() - eval_threats<Color::BLACK>();
  s += eval_passed_pawns<Color::WHITE>() - eval_passed_pawns<Color::BLACK>();

  return interpolate_score(s, pos->side_to_move());
}

#undef PRINT