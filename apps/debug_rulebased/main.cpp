#include "contrast/game_state.hpp"
#include "contrast/rules.hpp"
#include "contrast/move_list.hpp"
#include "contrast_ai/rule_based_policy.hpp"
#include "contrast_ai/random_policy.hpp"
#include <iostream>
#include <iomanip>

using namespace contrast;
using namespace contrast_ai;

void print_board_with_distances(const GameState& state) {
    std::cout << "\nBoard (y=0 at top, y=4 at bottom):\n";
    std::cout << "    0   1   2   3   4\n";
    std::cout << "  +---+---+---+---+---+\n";
    for (int y = 0; y < 5; ++y) {
        std::cout << y << " |";
        for (int x = 0; x < 5; ++x) {
            Player p = state.board().at(x, y).occupant;
            if (p == Player::Black) {
                int dist_to_goal = 4 - y;  // Black goal is y=4
                std::cout << " B" << dist_to_goal << "|";
            } else if (p == Player::White) {
                int dist_to_goal = y - 0;  // White goal is y=0
                std::cout << " W" << dist_to_goal << "|";
            } else {
                std::cout << " . |";
            }
        }
        std::cout << "\n  +---+---+---+---+---+\n";
    }
    std::cout << "Current player: " << (state.current_player() == Player::Black ? "Black" : "White") << "\n";
    std::cout << "(Numbers show distance to goal)\n";
}

void print_piece_positions(const GameState& state, Player player) {
    int goal_row = (player == Player::Black) ? 4 : 0;
    std::cout << "\n" << (player == Player::Black ? "Black" : "White") 
              << " pieces (goal: y=" << goal_row << "):\n";
    
    int min_dist = 100;
    int closest_x = -1, closest_y = -1;
    
    for (int x = 0; x < 5; ++x) {
        for (int y = 0; y < 5; ++y) {
            if (state.board().at(x, y).occupant == player) {
                int dist = std::abs(y - goal_row);
                std::cout << "  (" << x << "," << y << ") distance=" << dist;
                if (dist < min_dist) {
                    min_dist = dist;
                    closest_x = x;
                    closest_y = y;
                    std::cout << " <-- CLOSEST";
                }
                std::cout << "\n";
            }
        }
    }
    
    if (min_dist <= 2) {
        std::cout << "  ** NEAR GOAL: Closest piece at (" << closest_x << "," << closest_y 
                  << "), distance=" << min_dist << " **\n";
    }
}

void analyze_move_selection(const GameState& state, RuleBasedPolicy& policy) {
    Player current = state.current_player();
    Player opponent = (current == Player::Black) ? Player::White : Player::Black;
    
    std::cout << "\n=== MOVE SELECTION ANALYSIS ===\n";
    
    // Show current piece positions
    print_piece_positions(state, current);
    print_piece_positions(state, opponent);
    
    // Get legal moves
    MoveList legal_moves;
    Rules::legal_moves(state, legal_moves);
    
    std::cout << "\nLegal moves (" << legal_moves.size << " total):\n";
    
    // Analyze each move
    int goal_row = (current == Player::Black) ? 4 : 0;
    
    for (size_t i = 0; i < std::min(legal_moves.size, (size_t)10); ++i) {
        const auto& m = legal_moves[i];
        int delta_y = m.dy - m.sy;
        int forward = (current == Player::Black) ? delta_y : -delta_y;
        
        std::cout << "  " << i+1 << ". (" << m.sx << "," << m.sy << ") -> (" 
                  << m.dx << "," << m.dy << ")";
        std::cout << " [forward=" << forward << "]";
        
        int dist_before = std::abs(m.sy - goal_row);
        int dist_after = std::abs(m.dy - goal_row);
        std::cout << " dist:" << dist_before << "->" << dist_after;
        
        // Check if this is a winning move
        GameState next = state;
        next.apply_move(m);
        if (Rules::is_win(next, current)) {
            std::cout << " ** WINNING MOVE **";
        }
        
        std::cout << "\n";
    }
    
    if (legal_moves.size > 10) {
        std::cout << "  ... (" << (legal_moves.size - 10) << " more moves)\n";
    }
    
    // Get the actual chosen move
    Move chosen = policy.pick(state);
    
    std::cout << "\nCHOSEN MOVE: (" << chosen.sx << "," << chosen.sy << ") -> (" 
              << chosen.dx << "," << chosen.dy << ")";
    
    int delta_y = chosen.dy - chosen.sy;
    int forward = (current == Player::Black) ? delta_y : -delta_y;
    std::cout << " [forward=" << forward << "]";
    
    GameState next = state;
    next.apply_move(chosen);
    if (Rules::is_win(next, current)) {
        std::cout << " ** THIS IS A WIN! **";
    }
    
    std::cout << "\n";
}

void test_blocking_behavior() {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "TEST: Blocking opponent near goal (can't win immediately)\n";
    std::cout << std::string(60, '=') << "\n";
    
    // Setup a scenario where White is close to goal but Black can't win yet
    GameState state;
    
    // Clear board
    for (int x = 0; x < 5; ++x) {
        for (int y = 0; y < 5; ++y) {
            state.board().at(x, y).occupant = Player::None;
        }
    }
    
    // White very close to goal (can win in 1 move)
    state.board().at(2, 1).occupant = Player::White;
    
    // Black far from goal (can't win immediately)
    state.board().at(0, 2).occupant = Player::Black;
    state.board().at(4, 2).occupant = Player::Black;
    
    // Black starts first by default
    
    print_board_with_distances(state);
    
    RuleBasedPolicy black_policy;
    analyze_move_selection(state, black_policy);
    
    std::cout << "\nExpected: Black should try to block White at (2,1)\n";
    std::cout << "Possible blocking moves: move near (2,1), e.g., to (2,2), (1,1), (3,1)\n";
}

void test_race_to_goal() {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "TEST: Race to goal (both players close)\n";
    std::cout << std::string(60, '=') << "\n";
    
    GameState state;
    
    // Clear board
    for (int x = 0; x < 5; ++x) {
        for (int y = 0; y < 5; ++y) {
            state.board().at(x, y).occupant = Player::None;
        }
    }
    
    // Both players close to goal
    state.board().at(1, 3).occupant = Player::Black;  // Black: 1 move from goal (y=4)
    state.board().at(3, 1).occupant = Player::White;  // White: 1 move from goal (y=0)
    
    // Black starts first by default
    
    print_board_with_distances(state);
    
    RuleBasedPolicy black_policy;
    analyze_move_selection(state, black_policy);
    
    std::cout << "\nExpected: Black should move toward goal (y=4) to win\n";
}

void test_forward_movement() {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "TEST: Forward movement priority\n";
    std::cout << std::string(60, '=') << "\n";
    
    GameState state;
    // Use initial setup
    
    print_board_with_distances(state);
    
    RuleBasedPolicy black_policy;
    analyze_move_selection(state, black_policy);
    
    std::cout << "\nExpected: Black should move forward (increase y)\n";
    
    // Make Black's move
    Move black_move = black_policy.pick(state);
    state.apply_move(black_move);
    
    print_board_with_distances(state);
    
    RuleBasedPolicy white_policy;
    analyze_move_selection(state, white_policy);
    
    std::cout << "\nExpected: White should move forward (decrease y)\n";
}

int main() {
    std::cout << "RuleBased Policy Behavior Analysis\n";
    std::cout << "===================================\n";
    std::cout << "\nGame setup:\n";
    std::cout << "  Black: starts at y=0, goal at y=4 (move DOWN, increase y)\n";
    std::cout << "  White: starts at y=4, goal at y=0 (move UP, decrease y)\n";
    
    // Test 1: Forward movement
    test_forward_movement();
    
    // Test 2: Race to goal
    test_race_to_goal();
    
    // Test 3: Blocking behavior
    test_blocking_behavior();
    
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "Analysis complete\n";
    std::cout << std::string(60, '=') << "\n";
    
    return 0;
}
