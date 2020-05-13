#include "searcher.h"
#include "evaluator.h"
#include "movegen.h"
#include "timer.h"
#include <iostream>
#include <algorithm>
#include <sstream>

uint64_t Searcher::search(depth_t depth, const HashList& hl) {
  reset();
  hash_list.resize(hl.size() + MaxPly, 0);
  std::copy(hl.begin(), hl.end(), hash_list.begin());
  game_ply = (depth_t) hl.size();

  MoveGen movegen(pos);
  GameLine gl;
  ScoredMove best_move;
  RootMoveList root_movelist;

  nodes = tthits = 0;
  for (Move::Type * m_ptr = movegen.begin(); *m_ptr != Move::Type::NONE; m_ptr++)
    root_movelist.push_back({ *m_ptr, 0 });

  Timer timer;
  timer.start();
  int alpha = -Score::MATE_SCORE, beta = Score::MATE_SCORE;
  for (depth_t d = 1; d <= depth; ++d) {
    best_move = { Move::Type::NONE, -Score::MATE_SCORE };
    for (depth_t ply = 0; ply < MaxPly; ++ply) {
      allow_nullmove[ply] = true;
    }

    int score;

    for (RootMoveList::iterator m = root_movelist.begin();
    m != root_movelist.end(); ++m) {
      pos.make_move(m->move, gl);
      hash_list[game_ply] = pos.hash();

      score = -alpha_beta(-Score::MATE_SCORE, Score::MATE_SCORE, d - 1, 1);
      m->score = score;
      pos.unmake_move(m->move, gl);

      if (score > best_move.score) {
        best_move.move = m->move;
        best_move.score = score;

        timer.stop();
        uint64_t nps =  (nodes / (timer.get_elapsed_ms() / 1000.));
        os << "info depth " << d;
        os << " score cp " << (int)(best_move.score / 10);
        os << " nodes " << nodes;
        os << " nps " << nps;
        os << " tthits " << tthits;
        os << " pv " << extract_pv(best_move.move) << std::endl;
        timer.start();
      }
    }

    std::stable_sort(root_movelist.begin(), root_movelist.end(), SortMoveList());
  }
  os << "bestmove " << Move::to_str(best_move.move)
     << " nodes " << nodes << std::endl;
  return nodes;
}

int Searcher::alpha_beta(int alpha, int beta, depth_t depth, depth_t ply)
{
  ++nodes;
  // Mate distance pruning
  beta = std::min(beta, Score::MATE_SCORE - (int)ply - 1);
  if (alpha >= beta)
    return alpha;

  TranspositionTable::Entry* ttentry; // Transposition table entry for this position
  ttentry = ttable.probe(pos.hash());

  if (ttentry != nullptr) {
    if (ttentry->get_depth() >= depth) {
      ++tthits;
      if (ttentry->get_type() == TTScoreType::ExactScore)
        return ttentry->get_score(ply);

      if ((ttentry->get_type() == TTScoreType::BetaBound)
        && (ttentry->get_score(ply) >= beta))
        return ttentry->get_score(ply);

      if ((ttentry->get_type() == TTScoreType::AlphaBound)
        && (ttentry->get_score(ply) <= alpha))
        return ttentry->get_score(ply);
    }
  }

  if (is_draw(ply))
    return Score::DRAW_SCORE;

  int eval = Score::UNKNOWN_SCORE, score;
  if (ttentry != nullptr) {
    if (ttentry->get_eval() != Score::UNKNOWN_SCORE)
      eval = ttentry->get_eval();
    else {
      eval = Evaluator(pos).eval();
      ttentry->set_eval(eval);
    }
  }
  else {
    eval = Evaluator(pos).eval();
    ttable.record_eval(pos.hash(), eval);
  }

  assert(eval != Score::UNKNOWN_SCORE);

  if (depth == 0) {
    return eval;
  }

  // Null-Move Pruning
  if (allow_nullmove[ply]
    && !pos.checkers() && !Score::is_mate_score(beta) && (eval >= beta)
    && (depth >= NullMoveMinDepth))
  {
    bool do_null_move = true;

    // If this position was previously searched and the score was recorded
    // then we can use that score to determine whether a doing a null-move
    // would be worth it or not
    if (ttentry != nullptr) {
      assert(ttentry->get_type() != TTScoreType::EmptyScore);

      if ((ttentry->get_depth() >= (depth - NullMovePruningDepth))
        && (ttentry->get_type() != TTScoreType::BetaBound)
        && (ttentry->get_score(ply) < beta))
        do_null_move = false;
    }
    
    // If we are in an endgame, don't use null-move pruning
    if (do_null_move)
      if (pos.side_to_move() == Color::WHITE) {
        using P = Piece::PieceOfColor<Color::WHITE>;
        if (!pos.pieces(P::Pawn))
          do_null_move = false;
        else if (!(pos.pieces(Color::WHITE) ^ pos.pieces(P::Pawn) ^ pos.pieces(P::King)))
          do_null_move = false;
      }
      else if (pos.side_to_move() == Color::BLACK) {
        using P = Piece::PieceOfColor<Color::BLACK>;
        if (!pos.pieces(P::Pawn))
          do_null_move = false;
        else if (!(pos.pieces(Color::BLACK) ^ pos.pieces(P::Pawn) ^ pos.pieces(P::King)))
          do_null_move = false;
      }

    if (do_null_move) {
      // Don't allow two null moves in a row
      allow_nullmove[ply + 1] = false;

      // Okay, do nothing now... I mean do the null move
      GameLine gl;
      pos.make_null_move(gl);
      score = -alpha_beta(-beta, -beta + 1, depth - NullMovePruningDepth, ply + 1);
      pos.unmake_null_move(gl);
      allow_nullmove[ply + 1] = true;

      if (score >= beta) {
        if (Score::is_mate_score(score))
          score = beta;
        return score;
      }
    }
  }

  MoveGen movegen(pos);

  // Test if the current position is checkmate or stalemate
  if (movegen.size() == 0) {
    if (pos.checkers()) // Oops... Checkmate
      score = -(int)(Score::MATE_SCORE - (ply + 1));  // Checkmate score based on ply
    else
      score = Score::DRAW_SCORE; // Stalemate, draw

    ttable.record(pos.hash(), depth, score, eval,
      Move::Type::NONE, TTScoreType::ExactScore, ply);
    return score;
  }

  // Test if the current position is draw by fifty-move rule
  if (pos.half_move() >= 100)
    return Score::DRAW_SCORE; // Return stalemate score, aka contempt factor

  GameLine gl;
  bool pv_found = false;
  Move::Type best_move, hash_move = Move::Type::NONE;

  if (ttentry != nullptr) {
    if (ttentry->get_type() != TTScoreType::AlphaBound)
      hash_move = ttentry->get_best_move();
  }

  ScoredMoveList smlist;
  smlist.mlist = movegen.begin();
  movepicker.score_moves(smlist, ply, hash_move);
  for (int idx = 0; idx < movegen.size(); ++idx) {
    Move::Type m = movepicker.get_next_move(smlist, idx);

    pos.make_move(m, gl);
    hash_list[game_ply + ply] = pos.hash();

      if (pv_found) {
        score = -alpha_beta(-(alpha + 1), -alpha, depth - 1, ply + 1);
        if ((score > alpha) && (score < beta))
          score = -alpha_beta(-beta, -alpha, depth - 1, ply + 1);
      }
      else score = -alpha_beta(-beta, -alpha, depth - 1, ply + 1);

    pos.unmake_move(m, gl);

    if (score >= beta) {  // Oh yeah, cutoff
      movepicker.reg_beta_cutoff(smlist, idx, ply, depth);
      ttable.record(pos.hash(), depth, score, eval, m, TTScoreType::BetaBound, ply);
      return beta;
    }

    if (score > alpha) {
      alpha = score;
      pv_found = true;
      best_move = m;
    }
  }

  if (!pv_found)
    best_move = *movegen.begin();

  ttable.record(pos.hash(), depth, score, eval, best_move,
    pv_found ? TTScoreType::ExactScore : TTScoreType::AlphaBound, ply);
  return alpha;
}

int Searcher::qsearch(int alpha, int beta, int depth, int ply)
{
  return 0;
}

std::string Searcher::extract_pv(Move::Type root_move)
{
  std::stringstream ss;
  ss << Move::to_str(root_move) << ' ';
  Position p(pos);
  p.make_move(root_move, GameLine());

  TranspositionTable::Entry* e;
  e = ttable.probe(p.hash());
  if (e == nullptr)
    return Move::to_str(root_move);

  Move::Type m;
  while ((m = e->get_best_move()) != Move::Type::NONE) {
    assert(p.is_ok(m));

    ss << Move::to_str(m) << ' ';

    p.make_move(m, GameLine());
    e = ttable.probe(p.hash());

    if (e == nullptr)
      break;
  }
  
  return ss.str();
}
