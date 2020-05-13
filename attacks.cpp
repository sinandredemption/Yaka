#include "attacks.h"
#include "random.h"

namespace Attacks {
  uint64_t KnightAttacks[Square::SQ_NB];
  uint64_t KingAttacks[Square::SQ_NB];
  uint64_t RankMaskEx[Square::SQ_NB];
  uint64_t FileMaskEx[Square::SQ_NB];
  uint64_t DiagMaskEx[Square::SQ_NB];
  uint64_t ADiagMaskEx[Square::SQ_NB];
  uint64_t PawnAttacks[Color::COLOR_NB][Square::SQ_NB];
  uint64_t FrontSquares[Color::COLOR_NB][Square::SQ_NB] = { { 0 }, { 0 } };

  uint64_t RTables[0x16200]; // 708 kB
  uint64_t * RAttacks[Square::SQ_NB];
  uint64_t RMasks[Square::SQ_NB];
  uint64_t BTables[0x12C0]; // 37 kB
  uint64_t * BAttacks[Square::SQ_NB];
  uint64_t BMasks[Square::SQ_NB];
  uint64_t SqBetween[Square::SQ_NB][Square::SQ_NB];
  uint64_t LineBetween[Square::SQ_NB][Square::SQ_NB];
  uint64_t NeighbouringFiles[Square::FILE_NB] = { 0 };
  uint64_t PassedPawnMask[Color::COLOR_NB][Square::SQ_NB];
  uint64_t PawnAttackSpan[Color::COLOR_NB][Square::SQ_NB];

  void init()
  {
    Square::Type sq;
    for (sq = Square::A1; sq < Square::SQ_NB; ++sq) {
      using namespace Bitboard;
      RankMaskEx[sq] = RankMask[Square::rank_of(sq)] & ~sq_mask(sq);
      FileMaskEx[sq] = FileMask[Square::file_of(sq)] & ~sq_mask(sq);
      DiagMaskEx[sq] = DiagMask[Square::diag_of(sq)] & ~sq_mask(sq);
      ADiagMaskEx[sq] = ADiagMask[Square::adiag_of(sq)] & ~sq_mask(sq);
    }

    // Initialize all "fancy" magic bitboards
    RAttacks[0] = RTables;  // Set first offset
    BAttacks[0] = BTables;  // Set first offset

    for (sq = Square::A1; sq < Square::SQ_NB; ++sq) {
      using namespace Bitboard;
      init_piece(Piece::ROOK, sq);
      init_piece(Piece::BISHOP, sq);

      KingAttacks[sq] = king_attacks(sq_mask(sq));
      KnightAttacks[sq] = knight_attacks(sq_mask(sq));

      if (Square::rank_of(sq) != Square::RANK_8)
        FrontSquares[Color::WHITE][sq] =
        north_fill(RankMask[Square::rank_of(sq) + 1]);
      if (Square::rank_of(sq) != Square::RANK_1)
        FrontSquares[Color::BLACK][sq] =
        south_fill(RankMask[Square::rank_of(sq) - 1]);

      PassedPawnMask[Color::WHITE][sq] = north_fill(sq_mask(sq));
      PassedPawnMask[Color::BLACK][sq] = south_fill(sq_mask(sq));
      if (Square::file_of(sq) != Square::FILE_H) {
        NeighbouringFiles[Square::file_of(sq)] |= FileMask[Square::file_of(sq + 1)];
        PassedPawnMask[Color::WHITE][sq] |= north_fill(sq_mask(sq + 1));
        PassedPawnMask[Color::BLACK][sq] |= south_fill(sq_mask(sq + 1));
      }
      if (Square::file_of(sq) != Square::FILE_A) {
        NeighbouringFiles[Square::file_of(sq)] |= FileMask[Square::file_of(sq - 1)];
        PassedPawnMask[Color::WHITE][sq] |= north_fill(sq_mask(sq - 1));
        PassedPawnMask[Color::BLACK][sq] |= south_fill(sq_mask(sq - 1));
      }

      PassedPawnMask[Color::WHITE][sq] &= ~RankMask[Square::rank_of(sq)];
      PassedPawnMask[Color::BLACK][sq] &= ~RankMask[Square::rank_of(sq)];
      PawnAttackSpan[Color::WHITE][sq] = PassedPawnMask[Color::WHITE][sq] & NeighbouringFiles[Square::file_of(sq)];
      PawnAttackSpan[Color::BLACK][sq] = PassedPawnMask[Color::BLACK][sq] & NeighbouringFiles[Square::file_of(sq)];

      // Initialize pawn attacks
      PawnAttacks[Color::WHITE][sq] = pawn_attacks<Color::WHITE>(sq_mask(sq));
      PawnAttacks[Color::BLACK][sq] = pawn_attacks<Color::BLACK>(sq_mask(sq));

      for (Square::Type sq2 = Square::A1; sq2 < Square::SQ_NB; ++sq2) {
        SqBetween[sq][sq2] = LineBetween[sq][sq2] = 0;

        if (FileMaskEx[sq] & sq_mask(sq2)) {
          SqBetween[sq][sq2] = file_attacks(sq, sq_mask(sq2));
          SqBetween[sq][sq2] &= file_attacks(sq2, sq_mask(sq));
          assert(Square::file_of(sq) == Square::file_of(sq2));
          LineBetween[sq][sq2] = FileMask[Square::file_of(sq)];
        } else if (RankMaskEx[sq] & sq_mask(sq2)) {
          SqBetween[sq][sq2] = rank_attacks(sq, sq_mask(sq2));
          SqBetween[sq][sq2] &= rank_attacks(sq2, sq_mask(sq));
          assert(Square::rank_of(sq) == Square::rank_of(sq2));
          LineBetween[sq][sq2] = RankMask[Square::rank_of(sq)];
        } else if (DiagMaskEx[sq] & sq_mask(sq2)) {
          SqBetween[sq][sq2] = diag_attacks(sq, sq_mask(sq2));
          SqBetween[sq][sq2] &= diag_attacks(sq2, sq_mask(sq));
          assert(Square::diag_of(sq) == Square::diag_of(sq2));
          LineBetween[sq][sq2] = DiagMask[Square::diag_of(sq)];
        } else if (ADiagMaskEx[sq] & sq_mask(sq2)) {
          SqBetween[sq][sq2] = adiag_attacks(sq, sq_mask(sq2));
          SqBetween[sq][sq2] &= adiag_attacks(sq2, sq_mask(sq));
          assert(Square::adiag_of(sq) == Square::adiag_of(sq2));
          LineBetween[sq][sq2] = ADiagMask[Square::adiag_of(sq)];
        }

        assert(!(SqBetween[sq][sq2] & sq_mask(sq, sq2)));
      }
    }
  }


  void init_piece(Piece::PieceType pt, Square::Type sq)
  {
    uint64_t *Masks = pt == Piece::ROOK ? RMasks : BMasks;
    const uint64_t * Magics = pt == Piece::ROOK ? RMagics : BMagics;
    uint64_t ** Attacks = pt == Piece::ROOK ? RAttacks : BAttacks;
    uint64_t(*calc_attacks)(Square::Type, uint64_t) =
      pt == Piece::ROOK ? calc_rook_attacks : calc_bishop_attacks;
    const count_t * Shift = pt == Piece::ROOK ? RShift : BShift;

    Masks[sq] = calc_attacks(sq, 0) & ~Bitboard::sq_mask(sq);
    if (Square::file_of(sq) != Square::FILE_A)
      Masks[sq] &= ~Bitboard::FileAMask;
    if (Square::file_of(sq) != Square::FILE_H)
      Masks[sq] &= ~Bitboard::FileHMask;
    if (Square::rank_of(sq) != Square::RANK_1)
      Masks[sq] &= ~Bitboard::Rank1Mask;
    if (Square::rank_of(sq) != Square::RANK_8)
      Masks[sq] &= ~Bitboard::Rank8Mask;

    const size_t TableSize = 1ULL << (64 - Shift[sq]);
    std::memset(Attacks[sq], 0, TableSize * sizeof(uint64_t));

    uint64_t occ = 0;
    do {
      hash_t index = hash_t(((occ & Masks[sq]) * Magics[sq]) >> Shift[sq]);
      assert((Attacks[sq][index] == 0)
             || (Attacks[sq][index] == calc_attacks(sq, occ)));
      Attacks[sq][index] = calc_attacks(sq, occ);
    } while (occ = next_subset(Masks[sq], occ));

    if (sq < Square::H8)
      Attacks[sq + 1] = Attacks[sq] + TableSize;
  }
}