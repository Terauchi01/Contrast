#include <gtest/gtest.h>
#include "contrast/game_state.hpp"
#include "contrast/rules.hpp"
#include "contrast/move_list.hpp"
#include "contrast_ai/random_policy.hpp"
#include "contrast_ai/greedy_policy.hpp"
#include <iostream>

using namespace contrast;

// Helper function to play a game between two policies
template<typename PolicyBlack, typename PolicyWhite>
Player play_game(PolicyBlack& black_policy, PolicyWhite& white_policy, int max_turns = 200) {
  GameState state;
  
  for (int turn = 0; turn < max_turns; ++turn) {
    // Check win/loss
    if (Rules::is_loss(state, state.current_player())) {
      return (state.current_player() == Player::Black) ? Player::White : Player::Black;
    }
    
    // Check for win
    if (Rules::is_win(state, state.current_player())) {
      return state.current_player();
    }
    
    // Get move from current player's policy
    Move move;
    if (state.current_player() == Player::Black) {
      move = black_policy.pick(state);
    } else {
      move = white_policy.pick(state);
    }
    
    // Apply move (assumes the policy returns valid moves)
    state.apply_move(move);
  }
  
  // Max turns reached - draw
  return Player::None;
}

TEST(AITest, RandomPolicyCanPickMove) {
  GameState state;
  contrast_ai::RandomPolicy policy;
  
  Move move = policy.pick(state);
  
  // Should pick a valid move
  contrast::MoveList moves;
  Rules::legal_moves(state, moves);
  bool found = false;
  for (const auto& m : moves) {
    if (m.sx == move.sx && m.sy == move.sy && 
        m.dx == move.dx && m.dy == move.dy) {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);
}

TEST(AITest, GreedyPolicyCanPickMove) {
  GameState state;
  contrast_ai::GreedyPolicy policy;
  
  Move move = policy.pick(state);
  
  // Should pick a valid move
  contrast::MoveList moves;
  Rules::legal_moves(state, moves);
  bool found = false;
  for (const auto& m : moves) {
    if (m.sx == move.sx && m.sy == move.sy && 
        m.dx == move.dx && m.dy == move.dy) {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);
}

TEST(AITest, GreedyPolicyPrefersForwardMoves) {
  GameState state;
  contrast_ai::GreedyPolicy policy;
  
  // Play several moves as Black and check that they tend to go forward
  int forward_count = 0;
  int total_moves = 100;
  
  for (int i = 0; i < total_moves; ++i) {
    GameState test_state;
    Move move = policy.pick(test_state);
    
    // Check if move goes forward (row increases for Black)
    if (move.dy > move.sy) {
      forward_count++;
    }
  }
  
  // At least 70% should be forward moves
  EXPECT_GT(forward_count, total_moves * 0.7);
}

TEST(AITest, RandomVsRandomCanFinishGame) {
  contrast_ai::RandomPolicy black_policy;
  contrast_ai::RandomPolicy white_policy;
  
  Player winner = play_game(black_policy, white_policy);
  
  // Game should end (either Black, White, or Draw)
  // Just checking that it doesn't hang
  EXPECT_TRUE(winner == Player::Black || winner == Player::White || winner == Player::None);
}

TEST(AITest, GreedyVsRandomCanFinishGame) {
  contrast_ai::GreedyPolicy black_policy;
  contrast_ai::RandomPolicy white_policy;
  
  Player winner = play_game(black_policy, white_policy);
  
  // Game should end
  EXPECT_TRUE(winner == Player::Black || winner == Player::White || winner == Player::None);
}

TEST(AITest, GreedyVsGreedyCanFinishGame) {
  contrast_ai::GreedyPolicy black_policy;
  contrast_ai::GreedyPolicy white_policy;
  
  Player winner = play_game(black_policy, white_policy);
  
  // Game should end
  EXPECT_TRUE(winner == Player::Black || winner == Player::White || winner == Player::None);
  
  // Print result for debugging
  if (winner == Player::Black) {
    std::cout << "Greedy vs Greedy: Black wins\n";
  } else if (winner == Player::White) {
    std::cout << "Greedy vs Greedy: White wins\n";
  } else {
    std::cout << "Greedy vs Greedy: Draw\n";
  }
}

TEST(AITest, MultipleGamesStatistics) {
  const int num_games = 50;
  int black_wins = 0;
  int white_wins = 0;
  int draws = 0;
  
  for (int i = 0; i < num_games; ++i) {
    contrast_ai::GreedyPolicy black_policy;
    contrast_ai::RandomPolicy white_policy;
    
    Player winner = play_game(black_policy, white_policy);
    
    if (winner == Player::Black) black_wins++;
    else if (winner == Player::White) white_wins++;
    else draws++;
  }
  
  std::cout << "Greedy(Black) vs Random(White) - " << num_games << " games:\n";
  std::cout << "  Black wins: " << black_wins << " (" 
            << (100.0 * black_wins / num_games) << "%)\n";
  std::cout << "  White wins: " << white_wins << " (" 
            << (100.0 * white_wins / num_games) << "%)\n";
  std::cout << "  Draws: " << draws << " (" 
            << (100.0 * draws / num_games) << "%)\n";
  
  // Greedy should win more often than random
  EXPECT_GT(black_wins, white_wins);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
