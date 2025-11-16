#include "contrast/game_state.hpp"
#include "contrast/rules.hpp"
#include "contrast/move_list.hpp"
#include "contrast_ai/rule_based_policy.hpp"
#include "contrast_ai/greedy_policy.hpp"
#include "contrast_ai/random_policy.hpp"
#include <iostream>

using namespace contrast;
using namespace contrast_ai;

struct TestResult {
    int player1_wins = 0;
    int player2_wins = 0;
    int draws = 0;
    double avg_moves = 0.0;
    
    void print(const std::string& player1_name, const std::string& player2_name) const {
        int total = player1_wins + player2_wins + draws;
        std::cout << "\nResults: " << player1_name << " vs " << player2_name << "\n";
        std::cout << "  " << player1_name << " wins: " << player1_wins 
                  << " (" << (100.0 * player1_wins / total) << "%)\n";
        std::cout << "  " << player2_name << " wins: " << player2_wins 
                  << " (" << (100.0 * player2_wins / total) << "%)\n";
        std::cout << "  Draws: " << draws 
                  << " (" << (100.0 * draws / total) << "%)\n";
        std::cout << "  Average moves: " << avg_moves << "\n";
    }
};

template<typename Policy1, typename Policy2>
TestResult run_test(Policy1& p1, Policy2& p2, int num_games, bool p1_is_black) {
    TestResult result;
    int total_moves = 0;
    
    for (int game = 0; game < num_games; ++game) {
        GameState state;
        int moves = 0;
        const int MAX_MOVES = 1000;
        
        while (moves < MAX_MOVES) {
            if (Rules::is_win(state, Player::Black)) {
                if (p1_is_black) result.player1_wins++;
                else result.player2_wins++;
                break;
            }
            if (Rules::is_win(state, Player::White)) {
                if (p1_is_black) result.player2_wins++;
                else result.player1_wins++;
                break;
            }
            
            MoveList legal_moves;
            Rules::legal_moves(state, legal_moves);
            
            if (legal_moves.empty()) {
                Player winner = (state.current_player() == Player::Black) ? Player::White : Player::Black;
                if (winner == Player::Black) {
                    if (p1_is_black) result.player1_wins++;
                    else result.player2_wins++;
                } else {
                    if (p1_is_black) result.player2_wins++;
                    else result.player1_wins++;
                }
                break;
            }
            
            Move move;
            bool p1_turn = (state.current_player() == Player::Black) == p1_is_black;
            if (p1_turn) {
                move = p1.pick(state);
            } else {
                move = p2.pick(state);
            }
            
            state.apply_move(move);
            moves++;
        }
        
        if (moves >= MAX_MOVES) {
            result.draws++;
        }
        total_moves += moves;
    }
    
    result.avg_moves = (double)total_moves / num_games;
    return result;
}

int main() {
    const int NUM_GAMES = 1000;
    
    std::cout << "========================================\n";
    std::cout << "RuleBased Policy Analysis\n";
    std::cout << "Testing " << NUM_GAMES << " games each\n";
    std::cout << "========================================\n";
    
    // Test 1: RuleBased(Black) vs Greedy(White)
    {
        RuleBasedPolicy rb;
        GreedyPolicy greedy;
        auto result = run_test(rb, greedy, NUM_GAMES, true);
        result.print("RuleBased(Black)", "Greedy(White)");
    }
    
    // Test 2: RuleBased(White) vs Greedy(Black)
    {
        RuleBasedPolicy rb;
        GreedyPolicy greedy;
        auto result = run_test(rb, greedy, NUM_GAMES, false);
        result.print("RuleBased(White)", "Greedy(Black)");
    }
    
    std::cout << "\n";
    
    // Test 3: RuleBased(Black) vs Random(White)
    {
        RuleBasedPolicy rb;
        RandomPolicy random;
        auto result = run_test(rb, random, NUM_GAMES, true);
        result.print("RuleBased(Black)", "Random(White)");
    }
    
    // Test 4: RuleBased(White) vs Random(Black)
    {
        RuleBasedPolicy rb;
        RandomPolicy random;
        auto result = run_test(rb, random, NUM_GAMES, false);
        result.print("RuleBased(White)", "Random(Black)");
    }
    
    std::cout << "\n";
    
    // Test 5: Greedy(Black) vs Greedy(White)
    {
        GreedyPolicy g1, g2;
        auto result = run_test(g1, g2, NUM_GAMES, true);
        result.print("Greedy(Black)", "Greedy(White)");
    }
    
    // Test 6: RuleBased(Black) vs RuleBased(White)
    {
        RuleBasedPolicy rb1, rb2;
        auto result = run_test(rb1, rb2, NUM_GAMES, true);
        result.print("RuleBased(Black)", "RuleBased(White)");
    }
    
    std::cout << "\n========================================\n";
    std::cout << "Analysis Summary:\n";
    std::cout << "- If RuleBased(Black) has unusually high win rate\n";
    std::cout << "  compared to RuleBased(White), there's a Black bias\n";
    std::cout << "- If both colors perform similarly against same opponent,\n";
    std::cout << "  the policy is color-balanced\n";
    std::cout << "========================================\n";
    
    return 0;
}
