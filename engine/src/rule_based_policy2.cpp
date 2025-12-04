#include "contrast_ai/rule_based_policy2.hpp"
#include "contrast/rules.hpp"
#include "contrast/board.hpp"
#include "contrast/move_list.hpp"
#include <chrono>
#include <algorithm>
#include <limits>

namespace contrast_ai {

bool RuleBasedPolicy2::WinCheck(const contrast::GameState& s, const contrast::Move& m) {
  contrast::GameState next = s;
  next.apply_move(m);
  return contrast::Rules::is_win(next, s.current_player());
}
bool RuleBasedPolicy2::LoseCheck(){
    return false;
}
bool RuleBasedPolicy2::ProgressMove(){
    return false;
}
bool RuleBasedPolicy2::PromoteLeadPiece(){
    return false;
}
bool RuleBasedPolicy2::TileBoostMyPiece(){
    return false;
}
bool RuleBasedPolicy2::TileDisruptOpponent(){
    return false;
}
bool RuleBasedPolicy2::SelfJumpAccelerate(){
    return false;
}
bool RuleBasedPolicy2::AntiJumpDefense(){
    return false;
}
bool RuleBasedPolicy2::AvoidCluster(){
    return false;
}
bool RuleBasedPolicy2::EndgameOpponentBlock(){
    return false;
}

contrast::Move RuleBasedPolicy2::pick(const contrast::GameState& s) {
  contrast::MoveList moves;
  contrast::Rules::legal_moves(s, moves);
  
  if (moves.empty()) {
    return contrast::Move();
  }
  
  contrast::Player me = s.current_player();
  contrast::Player opp = (me == contrast::Player::Black) ? contrast::Player::White : contrast::Player::Black;
  
  // Priority 1: Check if I can win immediately
  for(int i = 0; i < moves.size; ++i) {
    const contrast::Move& m = moves[i];
    contrast::GameState next = s;
    next.apply_move(m);
    if (contrast::Rules::is_win(next, s.current_player())) { return moves[i]; }
  }
  
  // Priority 2: Block opponent's immediate winning move (distance = 1 only)
  int opp_min_dist = min_distance_to_empty_goal(s, opp);
  if (opp_min_dist == 1) {
    // Opponent can win next turn, must block
    contrast::MoveList block_moves;
    find_block_moves(s, opp, moves, block_moves);
    
    if (!block_moves.empty()) {
      return select_best_block_move(s, opp, block_moves);
    }
  }
  
  // Priority 3: Move rear pieces forward for multi-piece attack
  // Score all moves and pick the best one
  const contrast::Move* best = &moves[0];
  int best_score = score_move_to_empty_goal(s, moves[0], me);
  
  for (size_t i = 1; i < moves.size; ++i) {
    int score = score_move_to_empty_goal(s, moves[i], me);
    if (score > best_score) {
      best_score = score;
      best = &moves[i];
    }
  }
  
  return *best;
}

} // namespace contrast_ai
