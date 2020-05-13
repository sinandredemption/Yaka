#include "yaka.h"

namespace Program {
  std::string StartFen =
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
  std::string Name = "Yaka";
  std::string HardwareInfo = "x64";
#ifndef NDEBUG
  std::string Version = "0.0 Beta " + HardwareInfo;
#else
  std::string Version = "0.0 " + HardwareInfo;
#endif
  std::string Authors = "Syed Fahad";
  std::string info()
  {
    return Name + ' ' + Version + " by " + Authors;
  }
  std::string uci_info()
  {
    std::string out;
    out += "id name " + Name + ' ';
    out += Version + '\n';
    out += "id author " + Authors + '\n';
    return out;
  }
}

namespace Color {
  char to_char(Type c)
  {
    assert(c < COLOR_NB);
    return (c == WHITE) ? 'w' : 'b';
  }

  std::string to_str(Type c)
  {
    assert(c < COLOR_NB);
    return (c == WHITE) ? "White" : "Black";
  }
}

namespace Square {
  std::string to_str(Type sq)
  {
    assert(sq <= SQ_NB);
    if (sq == NONE)
      return "-";
    std::string out;
    out += file_of(sq) + int('a');
    out += rank_of(sq) + int('1');
    return out;
  }
}

namespace Piece {
  std::string PieceToChar = "PpNnBbRrQqKk ";
  std::string to_str(PieceType pc)
  {
    switch (pc) {
    case PAWN:
      return "Pawn";
    case KNIGHT:
      return "Knight";
    case BISHOP:
      return "Bishop";
    case ROOK:
      return "Rook";
    case QUEEN:
      return "Queen";
    case KING:
      return "King";
    default:
      return "ERROR";
    }
  }

  std::string PieceTypeToChar = "pnbrqk ";
  std::string to_str(Type pc)
  {
    std::string out;
    out += Color::to_str(color_of(pc));
    out += ' ';
    out += to_str(piece_type(pc));
    return out;
  }
}

namespace Castling {
  std::string to_str(Right cr)
  {
    assert(cr < CASTLING_RIGHT_NB);
    if (cr == NONE) return "-";
    std::string out;
    if (cr & WHITE_OO) out += 'K';
    if (cr & WHITE_OOO) out += 'Q';
    if (cr & BLACK_OO) out += 'k';
    if (cr & BLACK_OOO) out += 'q';
    return out;
  }
}

namespace Move {
  std::string to_str(Type m)
  {
    std::string out;
    out += Square::to_str(from_sq(m));
    out += Square::to_str(to_sq(m));
    if (is_move(m, Flags::PROMOTION))
      out += Piece::PieceTypeToChar[promotion_pc(m)];
    return out;
  }
}