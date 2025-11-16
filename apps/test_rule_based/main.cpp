#include "contrast/game_state.hpp"
#include "contrast/rules.hpp"
#include "contrast/move_list.hpp"
#include "contrast_ai/rule_based_policy.hpp"
#include "contrast_ai/greedy_policy.hpp"
#include "contrast_ai/random_policy.hpp"
#include <iostream>
#include <iomanip>
#include <functional>

using namespace contrast;
using namespace contrast_ai;

struct GameResult {
    Player winner;
    int num_moves;
};

template<typename BlackPolicy, typename WhitePolicy>
GameResult play_game(BlackPolicy& black_policy, WhitePolicy& white_policy) {
    GameState state;
    state.reset();
    
    const int MAX_MOVES = 500;
    int move_count = 0;
    
    while (move_count < MAX_MOVES) {
        MoveList moves;
        Rules::legal_moves(state, moves);
        
        if (moves.empty()) {
            return GameResult{
                (state.current_player() == Player::Black) ? Player::White : Player::Black,
                move_count
            };
        }
        
        if (Rules::is_win(state, Player::Black)) {
            return GameResult{Player::Black, move_count};
        }
        if (Rules::is_win(state, Player::White)) {
            return GameResult{Player::White, move_count};
        }
        
        Move move;
        if (state.current_player() == Player::Black) {
            move = black_policy.pick(state);
        } else {
            move = white_policy.pick(state);
        }
        
        state.apply_move(move);
        move_count++;
    }
    
    return GameResult{Player::None, move_count};
}

template<typename P1, typename P2>
void test_policies(const std::string& p1_name, P1& p1,
                   const std::string& p2_name, P2& p2,
                   int num_games) {
    int p1_wins = 0;
    int p2_wins = 0;
    int draws = 0;
    int total_moves = 0;
    
    std::cout << "\nTesting " << p1_name << " (Black) vs " << p2_name << " (White)\n";
    std::cout << "Playing " << num_games << " games...\n";
    
    for (int i = 0; i < num_games; ++i) {
        GameResult result = play_game(p1, p2);
        
        if (result.winner == Player::Black) p1_wins++;
        else if (result.winner == Player::White) p2_wins++;
        else draws++;
        
        total_moves += result.num_moves;
        
        if ((i + 1) % 100 == 0) {
            std::cout << "  Progress: " << (i + 1) << "/" << num_games << "\n";
        }
    }
    
    float p1_win_rate = 100.0f * p1_wins / num_games;
    float p2_win_rate = 100.0f * p2_wins / num_games;
    float draw_rate = 100.0f * draws / num_games;
    float avg_moves = static_cast<float>(total_moves) / num_games;
    
    std::cout << "\n--- Results ---\n";
    std::cout << p1_name << " (Black): " << p1_wins << " wins (" 
              << std::fixed << std::setprecision(1) << p1_win_rate << "%)\n";
    std::cout << p2_name << " (White): " << p2_wins << " wins (" 
              << p2_win_rate << "%)\n";
    std::cout << "Draws: " << draws << " (" << draw_rate << "%)\n";
    std::cout << "Average moves per game: " << avg_moves << "\n";
    std::cout << "---------------\n";
}

int main(int argc, char* argv[]) {
    int num_games = 1000;
    
    if (argc > 1) {
        num_games = std::stoi(argv[1]);
    }
    
    std::cout << "========================================\n";
    std::cout << "Rule-Based Policy Evaluation\n";
    std::cout << "========================================\n";
    
    RandomPolicy random;
    GreedyPolicy greedy;
    RuleBasedPolicy rule_based;
    
    // Test 1: RuleBased vs Random
    test_policies("RuleBased", rule_based, "Random", random, num_games);
    
    // Test 2: RuleBased vs Greedy
    test_policies("RuleBased", rule_based, "Greedy", greedy, num_games);
    
    // Test 3: Greedy vs Random (baseline)
    test_policies("Greedy", greedy, "Random", random, num_games);
    
    std::cout << "\n========================================\n";
    std::cout << "Evaluation Complete!\n";
    std::cout << "========================================\n";
    
    return 0;
}
