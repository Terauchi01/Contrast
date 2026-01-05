#include "contrast/game_state.hpp"
#include "contrast/rules.hpp"
#include "contrast/move_list.hpp"
#include "contrast_ai/random_policy.hpp"
#include "contrast_ai/greedy_policy.hpp"
#include "contrast_ai/rule_based_policy.hpp"
#include "contrast_ai/ntuple.hpp"
#include <iostream>
#include <memory>
#include <string>
#include <cstring>

using namespace contrast;
using namespace contrast_ai;

// Simple board printing for debugging
void print_board(const GameState& state) {
  const Board& b = state.board();
  int w = b.width();
  int h = b.height();
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      const Cell& c = b.at(x,y);
      char ch = '.';
      if (c.occupant == Player::Black) ch = 'B';
      else if (c.occupant == Player::White) ch = 'W';
      else ch = '.';
      std::cout << ch;
      if (c.tile != TileType::None) {
        // simple marker for tile presence
        std::cout << static_cast<int>(c.tile);
      } else {
        std::cout << ' ';
      }
      if (x < w-1) std::cout << " ";
    }
    std::cout << "\n";
  }
}

struct EvalStats {
  int black_wins = 0;
  int white_wins = 0;
  int draws = 0;
  int total_moves = 0;
  
  void record_game(Player winner, int moves) {
    if (winner == Player::Black) {
      black_wins++;
    } else if (winner == Player::White) {
      white_wins++;
    } else {
      draws++;
    }
    total_moves += moves;
  }
  
  int total_games() const {
    return black_wins + white_wins + draws;
  }
  
  double avg_moves() const {
    return total_games() > 0 ? static_cast<double>(total_moves) / total_games() : 0.0;
  }
  
  void print() const {
    int total = total_games();
    std::cout << "\nResults after " << total << " games:\n";
    std::cout << "  Black wins: " << black_wins << " (" 
              << (100.0 * black_wins / total) << "%)\n";
    std::cout << "  White wins: " << white_wins << " (" 
              << (100.0 * white_wins / total) << "%)\n";
    std::cout << "  Draws: " << draws << " (" 
              << (100.0 * draws / total) << "%)\n";
    std::cout << "  Average moves: " << avg_moves() << "\n";
  }
};

// Play one game between two policies
// If swap_colors is true, NTuple plays as White
Player play_game(NTuplePolicy& ntuple_policy, NTuplePolicy* opponent_ntuple, 
                 GreedyPolicy* opponent_greedy, RandomPolicy* opponent_random,
                 RuleBasedPolicy* opponent_rulebased,
                 int& moves, bool swap_colors = false, bool verbose = false) {
  GameState state;
  moves = 0;
  const int MAX_MOVES = 1000;
  
  while (moves < MAX_MOVES) {
    // Check win conditions
    if (Rules::is_win(state, Player::Black)) {
      return Player::Black;
    }
    if (Rules::is_win(state, Player::White)) {
      return Player::White;
    }
    
    MoveList legal_moves;
    Rules::legal_moves(state, legal_moves);
    
    if (legal_moves.empty()) {
      // No legal moves - current player loses
      return (state.current_player() == Player::Black) ? Player::White : Player::Black;
    }
    
    Move move;
    bool ntuple_turn = swap_colors ? (state.current_player() == Player::White) 
                                   : (state.current_player() == Player::Black);
    
    if (ntuple_turn) {
      move = ntuple_policy.pick(state);
    } else {
      if (opponent_ntuple) {
        move = opponent_ntuple->pick(state);
      } else if (opponent_greedy) {
        move = opponent_greedy->pick(state);
      } else if (opponent_rulebased) {
        move = opponent_rulebased->pick(state);
      } else if (opponent_random) {
        move = opponent_random->pick(state);
      }
    }
    
    if (verbose) {
      // Print simple move representation: player and coordinates
      Player p = state.current_player();
      std::cout << "Move " << (moves + 1) << ": " << (p == Player::Black ? "Black" : "White")
                << " -> sx=" << move.sx << ",sy=" << move.sy
                << ",dx=" << move.dx << ",dy=" << move.dy;
      if (move.place_tile) {
        std::cout << ", place tile at (" << move.tx << "," << move.ty << ") type=" << static_cast<int>(move.tile);
      }
      std::cout << "\n";
    }

    state.apply_move(move);

    if (verbose) {
      print_board(state);
      std::cout << "---\n";
    }
    moves++;
  }
  
  // Max moves reached - draw
  return Player::None;
}

int main(int argc, char** argv) {
  std::string weights_path = "ntuple_weights.bin";
  int num_games = 100;
  bool verbose = false;
  bool swap_colors = false;  // If true, NTuple plays as White
  std::string opponent = "greedy";  // "greedy", "random", "rulebased", or "ntuple"
  std::string opponent_weights_path = "";
  
  // Parse command line arguments
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--weights") == 0 && i + 1 < argc) {
      weights_path = argv[++i];
    } else if (std::strcmp(argv[i], "--games") == 0 && i + 1 < argc) {
      num_games = std::atoi(argv[++i]);
    } else if (std::strcmp(argv[i], "--opponent") == 0 && i + 1 < argc) {
      opponent = argv[++i];
    } else if (std::strcmp(argv[i], "--opponent-weights") == 0 && i + 1 < argc) {
      opponent_weights_path = argv[++i];
    } else if (std::strcmp(argv[i], "--swap-colors") == 0) {
      swap_colors = true;
    } else if (std::strcmp(argv[i], "--verbose") == 0) {
      verbose = true;
    } else if (std::strcmp(argv[i], "--help") == 0) {
      std::cout << "Usage: " << argv[0] << " [options]\n";
      std::cout << "Options:\n";
      std::cout << "  --weights <path>          Path to N-tuple weights file (default: ntuple_weights.bin)\n";
      std::cout << "  --games <n>               Number of games to play (default: 100)\n";
      std::cout << "  --opponent <type>         Opponent type: greedy, random, rulebased, or ntuple (default: greedy)\n";
      std::cout << "  --opponent-weights <path> Path to opponent's weights (if opponent=ntuple)\n";
      std::cout << "  --swap-colors             NTuple plays as White instead of Black\n";
      std::cout << "  --verbose                 Print progress during games\n";
      std::cout << "  --help                    Show this help message\n";
      return 0;
    }
  }
  
  // Load N-tuple policy for Black
  std::cout << "Loading N-tuple weights from: " << weights_path << "\n";
  NTuplePolicy ntuple_policy;
  if (!ntuple_policy.load(weights_path)) {
    std::cerr << "Error: Failed to load weights from " << weights_path << "\n";
    return 1;
  }
  std::cout << "Weights loaded successfully!\n";
  
  // Create opponent policy
  NTuplePolicy* opponent_ntuple = nullptr;
  GreedyPolicy* opponent_greedy = nullptr;
  RandomPolicy* opponent_random = nullptr;
  RuleBasedPolicy* opponent_rulebased = nullptr;
  
  std::string opponent_name;
  if (opponent == "ntuple") {
    opponent_ntuple = new NTuplePolicy();
    std::string opp_path = opponent_weights_path.empty() ? weights_path : opponent_weights_path;
    std::cout << "Loading opponent N-tuple weights from: " << opp_path << "\n";
    if (!opponent_ntuple->load(opp_path)) {
      std::cerr << "Error: Failed to load opponent weights\n";
      delete opponent_ntuple;
      return 1;
    }
    opponent_name = "N-tuple";
  } else if (opponent == "greedy") {
    opponent_greedy = new GreedyPolicy();
    opponent_name = "Greedy";
  } else if (opponent == "rulebased") {
    opponent_rulebased = new RuleBasedPolicy();
    opponent_name = "RuleBased";
  } else if (opponent == "random") {
    opponent_random = new RandomPolicy();
    opponent_name = "Random";
  } else {
    std::cerr << "Error: Unknown opponent type: " << opponent << "\n";
    return 1;
  }
  
  std::string ntuple_color = swap_colors ? "White" : "Black";
  std::string opponent_color = swap_colors ? "Black" : "White";
  
  std::cout << "\nEvaluating N-tuple (" << ntuple_color << ") vs " 
            << opponent_name << " (" << opponent_color << ")\n";
  std::cout << "========================================================\n";
  std::cout << "Number of games: " << num_games << "\n\n";
  
  EvalStats stats;
  int ntuple_wins = 0;
  int ntuple_losses = 0;
  int ntuple_draws = 0;
  
  for (int i = 0; i < num_games; ++i) {
    if ((i + 1) % 10 == 0 || verbose) {
      std::cout << "Playing game " << (i + 1) << "/" << num_games << "...\r" << std::flush;
    }
    
  int moves;
  Player winner = play_game(ntuple_policy, opponent_ntuple, opponent_greedy, opponent_random, opponent_rulebased, moves, swap_colors, verbose);
    stats.record_game(winner, moves);
    
    // Track NTuple-specific wins
    Player ntuple_player = swap_colors ? Player::White : Player::Black;
    if (winner == ntuple_player) {
      ntuple_wins++;
    } else if (winner != Player::None) {
      ntuple_losses++;
    } else {
      ntuple_draws++;
    }
  }
  
  std::cout << "\n";
  stats.print();
  
  // Print win rate from NTuple perspective
  int total = stats.total_games();
  std::cout << "\nN-tuple (" << ntuple_color << ") performance:\n";
  std::cout << "  Wins: " << ntuple_wins << " (" 
            << (100.0 * ntuple_wins / total) << "%)\n";
  std::cout << "  Losses: " << ntuple_losses << " (" 
            << (100.0 * ntuple_losses / total) << "%)\n";
  std::cout << "  Draws: " << ntuple_draws << " (" 
            << (100.0 * ntuple_draws / total) << "%)\n";
  
  // Clean up
  delete opponent_ntuple;
  delete opponent_greedy;
  delete opponent_random;
  delete opponent_rulebased;
  
  return 0;
}
