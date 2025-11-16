#include "contrast/game_state.hpp"
#include "contrast/rules.hpp"
#include "contrast/move_list.hpp"
#include "contrast_ai/ntuple.hpp"
#include "contrast_ai/greedy_policy.hpp"
#include <iostream>
#include <random>
#include <chrono>
#include <iomanip>

using namespace contrast;
using namespace contrast_ai;

struct TrainingConfig {
    int games_per_phase = 10000;
    int num_alternations = 20;  // Number of self/greedy alternations
    float learning_rate = 0.01f;
    float discount_factor = 0.9f;
    float exploration_rate = 0.2f;  // Higher exploration for alternating training
    int save_interval = 10000;
    std::string save_path = "ntuple_alternating.bin";
    std::string load_path = "";
};

// Epsilon-greedy move selection
Move select_move_epsilon_greedy(
    const GameState& state,
    NTupleNetwork& network,
    float epsilon,
    std::mt19937& rng
) {
    MoveList moves;
    Rules::legal_moves(state, moves);
    
    if (moves.empty()) {
        return Move{};
    }
    
    // Exploration: random move
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    if (dist(rng) < epsilon) {
        std::uniform_int_distribution<int> move_dist(0, static_cast<int>(moves.size - 1));
        return moves[move_dist(rng)];
    }
    
    // Exploitation: best move according to network
    Move best_move = moves[0];
    float best_value = -std::numeric_limits<float>::infinity();
    
    for (const auto& move : moves) {
        GameState next_state = state;
        next_state.apply_move(move);
        float value = network.evaluate(next_state);
        
        if (value > best_value) {
            best_value = value;
            best_move = move;
        }
    }
    
    return best_move;
}

struct GameResult {
    std::vector<GameState> states;
    std::vector<Player> players;
    Player winner;
    int num_moves;
};

GameResult play_training_game(
    NTupleNetwork& network,
    const TrainingConfig& config,
    GreedyPolicy* greedy_opponent,
    std::mt19937& rng
) {
    GameResult result;
    GameState state;
    state.reset();
    
    const int MAX_MOVES = 500;
    int move_count = 0;
    
    while (move_count < MAX_MOVES) {
        result.states.push_back(state);
        result.players.push_back(state.current_player());
        
        MoveList moves;
        Rules::legal_moves(state, moves);
        
        if (moves.empty()) {
            result.winner = (state.current_player() == Player::Black) 
                ? Player::White : Player::Black;
            result.num_moves = move_count;
            return result;
        }
        
        if (Rules::is_win(state, Player::Black)) {
            result.winner = Player::Black;
            result.num_moves = move_count;
            return result;
        }
        if (Rules::is_win(state, Player::White)) {
            result.winner = Player::White;
            result.num_moves = move_count;
            return result;
        }
        
        Move move;
        if (greedy_opponent != nullptr) {
            // NTuple vs Greedy: NTuple is Black, Greedy is White
            if (state.current_player() == Player::Black) {
                move = select_move_epsilon_greedy(state, network, config.exploration_rate, rng);
            } else {
                move = greedy_opponent->pick(state);
            }
        } else {
            // Self-play
            move = select_move_epsilon_greedy(state, network, config.exploration_rate, rng);
        }
        
        state.apply_move(move);
        move_count++;
    }
    
    result.winner = Player::None;
    result.num_moves = move_count;
    return result;
}

void td_learn_from_game(
    NTupleNetwork& network,
    const GameResult& result,
    const TrainingConfig& config,
    bool is_vs_greedy
) {
    if (result.states.empty()) return;
    
    for (int i = result.states.size() - 1; i >= 0; --i) {
        const auto& state = result.states[i];
        Player player = result.players[i];
        
        // If playing vs Greedy, only update NTuple's moves (Black)
        if (is_vs_greedy && player != Player::Black) {
            continue;
        }
        
        float target = 0.0f;
        if (result.winner == player) {
            target = 1.0f;
        } else if (result.winner != Player::None && result.winner != player) {
            target = -1.0f;
        }
        
        network.td_update(state, target, config.learning_rate);
    }
}

void train_phase(
    NTupleNetwork& network,
    const TrainingConfig& config,
    bool vs_greedy,
    int phase_num,
    std::mt19937& rng
) {
    GreedyPolicy* greedy_opponent = nullptr;
    if (vs_greedy) {
        greedy_opponent = new GreedyPolicy();
    }
    
    std::string phase_name = vs_greedy ? "Greedy" : "Self-play";
    std::cout << "\n=== Phase " << phase_num << ": " << phase_name 
              << " (" << config.games_per_phase << " games) ===\n";
    
    auto start_time = std::chrono::steady_clock::now();
    
    int black_wins = 0;
    int white_wins = 0;
    int draws = 0;
    float total_moves = 0;
    
    for (int game = 1; game <= config.games_per_phase; ++game) {
        GameResult result = play_training_game(network, config, greedy_opponent, rng);
        td_learn_from_game(network, result, config, vs_greedy);
        
        if (result.winner == Player::Black) black_wins++;
        else if (result.winner == Player::White) white_wins++;
        else draws++;
        total_moves += result.num_moves;
        
        if (game % 1000 == 0) {
            auto current_time = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                current_time - start_time).count();
            
            float avg_moves = total_moves / game;
            float games_per_sec = game / static_cast<float>(elapsed + 1);
            
            std::cout << "  Game " << std::setw(5) << game << "/" << config.games_per_phase
                      << " | B:" << std::setw(4) << black_wins
                      << " W:" << std::setw(4) << white_wins
                      << " D:" << std::setw(4) << draws
                      << " | Avg moves: " << std::setw(5) << std::fixed << std::setprecision(1) << avg_moves
                      << " | " << std::setw(5) << std::setprecision(1) << games_per_sec << " games/s\n";
        }
    }
    
    auto end_time = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        end_time - start_time).count();
    
    std::cout << "Phase " << phase_num << " complete (" << elapsed << "s):\n";
    std::cout << "  Black: " << black_wins << " (" 
              << (100.0f * black_wins / config.games_per_phase) << "%)\n";
    std::cout << "  White: " << white_wins << " (" 
              << (100.0f * white_wins / config.games_per_phase) << "%)\n";
    std::cout << "  Draws: " << draws << " (" 
              << (100.0f * draws / config.games_per_phase) << "%)\n";
    
    if (greedy_opponent != nullptr) {
        delete greedy_opponent;
    }
}

void train_alternating(const TrainingConfig& config) {
    NTupleNetwork network;
    
    if (!config.load_path.empty()) {
        std::cout << "Loading existing weights from: " << config.load_path << "\n";
        try {
            network.load(config.load_path);
            std::cout << "Weights loaded successfully!\n";
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to load weights: " << e.what() << "\n";
            std::cerr << "Starting with random weights instead.\n";
        }
    }
    
    std::mt19937 rng(std::random_device{}());
    
    std::cout << "\n========================================\n";
    std::cout << "Alternating Training Configuration\n";
    std::cout << "========================================\n";
    std::cout << "Games per phase: " << config.games_per_phase << "\n";
    std::cout << "Number of alternations: " << config.num_alternations << "\n";
    std::cout << "Total games: " << (config.games_per_phase * config.num_alternations) << "\n";
    std::cout << "Learning rate: " << config.learning_rate << "\n";
    std::cout << "Exploration rate: " << config.exploration_rate << "\n";
    std::cout << "Save interval: " << config.save_interval << "\n";
    std::cout << "Save path: " << config.save_path << "\n";
    std::cout << "========================================\n";
    
    auto global_start = std::chrono::steady_clock::now();
    int total_games = 0;
    
    for (int i = 0; i < config.num_alternations; ++i) {
        bool vs_greedy = (i % 2 == 1);  // Odd phases: vs Greedy, Even phases: Self-play
        train_phase(network, config, vs_greedy, i + 1, rng);
        
        total_games += config.games_per_phase;
        
        // Save checkpoint
        if (total_games % config.save_interval == 0) {
            std::string checkpoint_path = config.save_path + "." + std::to_string(total_games);
            network.save(checkpoint_path);
            std::cout << "\nSaved checkpoint: " << checkpoint_path << "\n";
        }
    }
    
    // Final save
    network.save(config.save_path);
    
    auto global_end = std::chrono::steady_clock::now();
    auto total_elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        global_end - global_start).count();
    
    std::cout << "\n========================================\n";
    std::cout << "Training Complete!\n";
    std::cout << "========================================\n";
    std::cout << "Total time: " << total_elapsed << " seconds\n";
    std::cout << "Total games: " << total_games << "\n";
    std::cout << "Final weights saved to: " << config.save_path << "\n";
    std::cout << "========================================\n";
}

int main(int argc, char* argv[]) {
    TrainingConfig config;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--games-per-phase" && i + 1 < argc) {
            config.games_per_phase = std::stoi(argv[++i]);
        } else if (arg == "--alternations" && i + 1 < argc) {
            config.num_alternations = std::stoi(argv[++i]);
        } else if (arg == "--lr" && i + 1 < argc) {
            config.learning_rate = std::stof(argv[++i]);
        } else if (arg == "--epsilon" && i + 1 < argc) {
            config.exploration_rate = std::stof(argv[++i]);
        } else if (arg == "--save-interval" && i + 1 < argc) {
            config.save_interval = std::stoi(argv[++i]);
        } else if (arg == "--output" && i + 1 < argc) {
            config.save_path = argv[++i];
        } else if (arg == "--load" && i + 1 < argc) {
            config.load_path = argv[++i];
        } else if (arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [options]\n";
            std::cout << "Alternating training: Self-play and Greedy opponent phases\n\n";
            std::cout << "Options:\n";
            std::cout << "  --games-per-phase N   Games per training phase (default: 10000)\n";
            std::cout << "  --alternations N      Number of phase alternations (default: 20)\n";
            std::cout << "  --lr RATE             Learning rate (default: 0.01)\n";
            std::cout << "  --epsilon EPS         Exploration rate (default: 0.2)\n";
            std::cout << "  --save-interval N     Save checkpoint every N games (default: 10000)\n";
            std::cout << "  --output PATH         Output file path (default: ntuple_alternating.bin)\n";
            std::cout << "  --load PATH           Load existing weights before training\n";
            std::cout << "  --help                Show this help message\n";
            std::cout << "\nTraining schedule:\n";
            std::cout << "  Phase 1: Self-play\n";
            std::cout << "  Phase 2: vs Greedy\n";
            std::cout << "  Phase 3: Self-play\n";
            std::cout << "  Phase 4: vs Greedy\n";
            std::cout << "  ... (repeats)\n";
            return 0;
        }
    }
    
    train_alternating(config);
    return 0;
}
