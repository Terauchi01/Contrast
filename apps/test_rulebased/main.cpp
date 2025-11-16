#include "contrast/game_state.hpp"
#include "contrast/rules.hpp"
#include "contrast/move_list.hpp"
#include "contrast_ai/rule_based_policy.hpp"
#include "contrast_ai/random_policy.hpp"
#include <iostream>

using namespace contrast;
using namespace contrast_ai;

void print_board(const GameState& state) {
    std::cout << "\nBoard (y=0 at top, y=4 at bottom):\n";
    for (int y = 0; y < 5; ++y) {
        std::cout << "y=" << y << ": ";
        for (int x = 0; x < 5; ++x) {
            Player p = state.board().at(x, y).occupant;
            if (p == Player::Black) std::cout << "B ";
            else if (p == Player::White) std::cout << "W ";
            else std::cout << ". ";
        }
        std::cout << "\n";
    }
    std::cout << "Current player: " << (state.current_player() == Player::Black ? "Black" : "White") << "\n";
}

void print_move(const Move& m) {
    std::cout << "  Move: (" << m.sx << "," << m.sy << ") -> (" << m.dx << "," << m.dy << ")";
    std::cout << " [delta_y=" << (m.dy - m.sy) << "]";
    if (m.place_tile) {
        std::cout << " + tile at (" << m.tx << "," << m.ty << ")";
    }
    std::cout << "\n";
}

// Play one game with detailed logging
Player play_game_verbose(RuleBasedPolicy& black_policy, RuleBasedPolicy& white_policy, 
                         int& moves, int max_moves_to_log = 10) {
    GameState state;
    moves = 0;
    const int MAX_MOVES = 1000;
    
    std::cout << "\n========== Game Start ==========\n";
    std::cout << "Black goal: y=4 (bottom), starts at y=0 (top)\n";
    std::cout << "White goal: y=0 (top), starts at y=4 (bottom)\n";
    
    print_board(state);
    
    while (moves < MAX_MOVES) {
        // Check win conditions
        if (Rules::is_win(state, Player::Black)) {
            std::cout << "\n*** Black WINS! ***\n";
            return Player::Black;
        }
        if (Rules::is_win(state, Player::White)) {
            std::cout << "\n*** White WINS! ***\n";
            return Player::White;
        }
        
        MoveList legal_moves;
        Rules::legal_moves(state, legal_moves);
        
        if (legal_moves.empty()) {
            Player loser = state.current_player();
            Player winner = (loser == Player::Black) ? Player::White : Player::Black;
            std::cout << "\n*** No legal moves for " 
                      << (loser == Player::Black ? "Black" : "White")
                      << " - " << (winner == Player::Black ? "Black" : "White") 
                      << " WINS! ***\n";
            return winner;
        }
        
        Move move;
        if (state.current_player() == Player::Black) {
            move = black_policy.pick(state);
        } else {
            move = white_policy.pick(state);
        }
        
        if (moves < max_moves_to_log) {
            std::cout << "\nMove " << (moves + 1) << " - " 
                      << (state.current_player() == Player::Black ? "Black" : "White") 
                      << ":\n";
            print_move(move);
        }
        
        state.apply_move(move);
        moves++;
        
        if (moves < max_moves_to_log) {
            print_board(state);
        }
    }
    
    std::cout << "\n*** DRAW (max moves reached) ***\n";
    return Player::None;
}

// Play multiple games and collect statistics
void test_rulebased_vs_rulebased(int num_games) {
    RuleBasedPolicy black_policy;
    RuleBasedPolicy white_policy;
    
    int black_wins = 0;
    int white_wins = 0;
    int draws = 0;
    int total_moves = 0;
    
    std::cout << "\n======================================\n";
    std::cout << "Testing RuleBased (Black) vs RuleBased (White)\n";
    std::cout << "Number of games: " << num_games << "\n";
    std::cout << "======================================\n";
    
    // Play first game with verbose logging
    int moves;
    Player winner = play_game_verbose(black_policy, white_policy, moves, 20);
    
    if (winner == Player::Black) black_wins++;
    else if (winner == Player::White) white_wins++;
    else draws++;
    total_moves += moves;
    
    std::cout << "\nFirst game result: ";
    if (winner == Player::Black) std::cout << "Black wins";
    else if (winner == Player::White) std::cout << "White wins";
    else std::cout << "Draw";
    std::cout << " in " << moves << " moves\n";
    
    // Play remaining games without verbose logging
    for (int i = 1; i < num_games; ++i) {
        GameState state;
        int game_moves = 0;
        const int MAX_MOVES = 1000;
        Player game_winner = Player::None;
        
        while (game_moves < MAX_MOVES) {
            if (Rules::is_win(state, Player::Black)) {
                game_winner = Player::Black;
                break;
            }
            if (Rules::is_win(state, Player::White)) {
                game_winner = Player::White;
                break;
            }
            
            MoveList legal_moves;
            Rules::legal_moves(state, legal_moves);
            
            if (legal_moves.empty()) {
                game_winner = (state.current_player() == Player::Black) ? Player::White : Player::Black;
                break;
            }
            
            Move move;
            if (state.current_player() == Player::Black) {
                move = black_policy.pick(state);
            } else {
                move = white_policy.pick(state);
            }
            
            state.apply_move(move);
            game_moves++;
        }
        
        if (game_winner == Player::Black) black_wins++;
        else if (game_winner == Player::White) white_wins++;
        else draws++;
        total_moves += game_moves;
    }
    
    // Print summary
    std::cout << "\n======================================\n";
    std::cout << "Results after " << num_games << " games:\n";
    std::cout << "  Black wins: " << black_wins << " (" 
              << (100.0 * black_wins / num_games) << "%)\n";
    std::cout << "  White wins: " << white_wins << " (" 
              << (100.0 * white_wins / num_games) << "%)\n";
    std::cout << "  Draws: " << draws << " (" 
              << (100.0 * draws / num_games) << "%)\n";
    std::cout << "  Average moves: " << (double)total_moves / num_games << "\n";
    std::cout << "======================================\n";
}

int main(int argc, char** argv) {
    int num_games = 100;
    
    if (argc > 1) {
        num_games = std::atoi(argv[1]);
    }
    
    test_rulebased_vs_rulebased(num_games);
    
    return 0;
}
