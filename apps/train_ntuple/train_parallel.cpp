#include "contrast/game_state.hpp"
#include "contrast/rules.hpp"
#include "contrast/move_list.hpp"
#include "contrast_ai/ntuple.hpp"
#include "contrast_ai/greedy_policy.hpp"
#include "contrast_ai/rule_based_policy.hpp"
#include <iostream>
#include <random>
#include <chrono>
#include <iomanip>
#include <deque>
#include <algorithm>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <queue>
#include <fstream>
#include <cstdlib>

using namespace contrast;
using namespace contrast_ai;

/**
 * 学習の設定パラメータ
 */
struct TrainingConfig {
    int num_games = 10000;
    float learning_rate = 0.01f;
    float exploration_rate = 0.1f;
    float opponent_exploration_rate = 0.0f;  // 対戦相手の探索率(0.0=greedy)
    int save_interval = 1000;
    int num_worker_threads = 4;  // ゲームプレイ用のワーカースレッド数
    std::string save_path = "ntuple_weights.bin";
    std::string load_path = "";
};

/**
 * 1ゲームの結果を保持する構造体
 */
struct GameResult {
    std::vector<GameState> states;
    std::vector<Player> players;
    Player winner;
    int num_moves;
};

/**
 * スレッドセーフなゲーム結果キュー
 */
class GameResultQueue {
private:
    std::queue<GameResult> queue_;
    mutable std::mutex mutex_;  // mutable allows locking in const methods
    std::condition_variable cv_;
    bool done_ = false;
    
public:
    void push(GameResult&& result) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(std::move(result));
        cv_.notify_one();
    }
    
    bool pop(GameResult& result) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return !queue_.empty() || done_; });
        
        if (queue_.empty()) {
            return false;  // No more results
        }
        
        result = std::move(queue_.front());
        queue_.pop();
        return true;
    }
    
    void set_done() {
        std::lock_guard<std::mutex> lock(mutex_);
        done_ = true;
        cv_.notify_all();
    }
    
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }
};

/**
 * スレッドセーフなNTupleネットワークラッパー
 * 
 * 複数のワーカースレッドが同時にネットワークを読み取り、
 * 更新スレッドが排他的に重みを更新する
 */
class SharedNTupleNetwork {
private:
    NTupleNetwork network_;
    mutable std::mutex mutex_;  // mutable allows locking in const methods
    
public:
    SharedNTupleNetwork() = default;
    
    SharedNTupleNetwork(const SharedNTupleNetwork&) = delete;
    SharedNTupleNetwork& operator=(const SharedNTupleNetwork&) = delete;
    
    // 読み取り専用：評価（複数スレッド同時アクセス可能）
    float evaluate(const GameState& state) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return network_.evaluate(state);
    }
    
    // 書き込み：重み更新（排他的アクセス）
    void td_update(const GameState& state, float target, float learning_rate) {
        std::lock_guard<std::mutex> lock(mutex_);
        network_.td_update(state, target, learning_rate);
    }
    
    // ファイル操作
    void save(const std::string& filename) const {
        std::lock_guard<std::mutex> lock(mutex_);
        network_.save(filename);
    }
    
    void load(const std::string& filename) {
        std::lock_guard<std::mutex> lock(mutex_);
        network_.load(filename);
    }
    
    size_t num_tuples() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return network_.num_tuples();
    }
    
    size_t num_weights() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return network_.num_weights();
    }
};

/**
 * 学習率スケジュール関数
 */
float calculate_learning_rate(int current_game, int total_games, float lr_max = 0.1f, float lr_min = 0.005f) {
    float progress = static_cast<float>(current_game - 1) / static_cast<float>(total_games - 1);
    float k = 19.0f;
    float lr = lr_min + (lr_max - lr_min) / (1.0f + k * progress * progress);
    return lr;
}

/**
 * ε-greedy法による手の選択
 */
Move select_move_epsilon_greedy(
    const GameState& state,
    const SharedNTupleNetwork& network,
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
        // Negamax: opponent's value = -our value
        float value = -network.evaluate(next_state);
        
        if (value > best_value) {
            best_value = value;
            best_move = move;
        }
    }
    
    return best_move;
}

/**
 * 学習用に1ゲームをプレイして軌跡を記録
 * 学習プレイヤー(Black)と対戦相手(White)で異なる探索率を使用
 */
GameResult play_training_game(
    const SharedNTupleNetwork& network,
    float learner_epsilon,      // 学習プレイヤー(Black)の探索率
    float opponent_epsilon,     // 対戦相手(White)の探索率
    std::mt19937& rng
) {
    GameResult result;
    GameState state;
    state.reset();
    
    const int MAX_MOVES = 500;
    int move_count = 0;
    
    while (move_count < MAX_MOVES) {
        // Save state before move
        result.states.push_back(state);
        result.players.push_back(state.current_player());
        
        // Check if game is over
        MoveList moves;
        Rules::legal_moves(state, moves);
        
        if (moves.empty()) {
            // No legal moves - opponent wins
            result.winner = (state.current_player() == Player::Black) 
                ? Player::White : Player::Black;
            result.num_moves = move_count;
            return result;
        }
        
        // Check win condition
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
        
        // Select and apply move with appropriate exploration rate
        float current_epsilon = (state.current_player() == Player::Black) 
            ? learner_epsilon : opponent_epsilon;
        Move move = select_move_epsilon_greedy(state, network, current_epsilon, rng);
        state.apply_move(move);
        move_count++;
    }
    
    // Max moves reached - draw
    result.winner = Player::None;
    result.num_moves = move_count;
    return result;
}

/**
 * ワーカースレッド：ゲームをプレイして結果をキューに追加
 */
void worker_thread(
    int worker_id,
    SharedNTupleNetwork& network,
    GameResultQueue& result_queue,
    std::atomic<int>& games_completed,
    int target_games,
    float learner_epsilon,      // 学習プレイヤー(Black)の探索率
    float opponent_epsilon      // 対戦相手(White)の探索率
) {
    std::mt19937 rng(std::random_device{}() + worker_id);
    
    std::cout << "[Worker " << worker_id << "] Started\n";
    
    int worker_games = 0;
    auto worker_start = std::chrono::steady_clock::now();
    
    while (true) {
        int game_num = games_completed.fetch_add(1);
        if (game_num >= target_games) {
            break;
        }
        
        // Debug: Measure game play time
        auto game_start = std::chrono::steady_clock::now();
        
        // Play game
        GameResult result = play_training_game(network, learner_epsilon, opponent_epsilon, rng);
        
        auto game_end = std::chrono::steady_clock::now();
        auto game_duration = std::chrono::duration_cast<std::chrono::milliseconds>(game_end - game_start).count();
        
        // Push result to queue
        result_queue.push(std::move(result));
        
        worker_games++;
        
        // Debug output every 10 games
        if (worker_games % 10 == 0) {
            auto worker_elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - worker_start).count();
            float worker_rate = worker_games / static_cast<float>(worker_elapsed + 1);
            std::cout << "[Worker " << worker_id << "] Games: " << worker_games 
                      << " | Last game: " << game_duration << "ms (" << result.num_moves << " moves)"
                      << " | Rate: " << std::fixed << std::setprecision(2) << worker_rate << " g/s"
                      << " | Queue: " << result_queue.size() << "\n";
        }
    }
    
    auto worker_end = std::chrono::steady_clock::now();
    auto total_time = std::chrono::duration_cast<std::chrono::seconds>(worker_end - worker_start).count();
    std::cout << "[Worker " << worker_id << "] Finished - Played " << worker_games 
              << " games in " << total_time << "s (" 
              << (worker_games / static_cast<float>(total_time + 1)) << " g/s)\n";
}

/**
 * 更新スレッド：キューからゲーム結果を取り出して重みを更新
 */
void updater_thread(
    SharedNTupleNetwork& network,
    GameResultQueue& result_queue,
    std::atomic<int>& games_processed,
    int total_games,
    const TrainingConfig& config
) {
    std::cout << "[Updater] Started\n";
    
    int black_wins = 0;
    int white_wins = 0;
    int draws = 0;
    float total_moves = 0;
    
    auto start_time = std::chrono::steady_clock::now();
    auto last_report_time = start_time;
    int games_since_report = 0;
    float total_update_time_ms = 0;
    float total_wait_time_ms = 0;
    
    while (true) {
        auto pop_start = std::chrono::steady_clock::now();
        
        GameResult result;
        if (!result_queue.pop(result)) {
            // Queue is done and empty
            break;
        }
        
        auto pop_end = std::chrono::steady_clock::now();
        auto wait_time = std::chrono::duration_cast<std::chrono::milliseconds>(pop_end - pop_start).count();
        total_wait_time_ms += wait_time;
        
        int current_game = games_processed.fetch_add(1) + 1;
        
        // Calculate current learning rate
        float current_lr = calculate_learning_rate(current_game, total_games);
        
        auto update_start = std::chrono::steady_clock::now();
        
        // TD learning from game result
        int num_updates = 0;
        for (int i = result.states.size() - 1; i >= 0; --i) {
            const auto& state = result.states[i];
            Player player = result.players[i];
            
            // Determine target value based on final outcome
            float target = 0.0f;
            if (result.winner == player) {
                target = 1.0f;  // This player won
            } else if (result.winner != Player::None && result.winner != player) {
                target = -1.0f; // This player lost
            }
            
            // Update this state's value toward the target
            network.td_update(state, target, current_lr);
            num_updates++;
        }
        
        // Measure update time
        auto update_end = std::chrono::steady_clock::now();
        auto update_time = std::chrono::duration_cast<std::chrono::milliseconds>(update_end - update_start).count();
        total_update_time_ms += update_time;
        games_since_report++;
        
        // Update statistics
        if (result.winner == Player::Black) black_wins++;
        else if (result.winner == Player::White) white_wins++;
        else draws++;
        total_moves += result.num_moves;
        
        // Progress reporting
        if (current_game % 100 == 0) {
            auto current_time = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                current_time - start_time).count();
            
            // Calculate timing statistics for the last 100 games
            float avg_wait_time = total_wait_time_ms / games_since_report;
            float avg_update_time = total_update_time_ms / games_since_report;
            float avg_total_time = avg_wait_time + avg_update_time;
            
            float avg_moves = total_moves / current_game;
            float games_per_sec = current_game / static_cast<float>(elapsed + 1);
            float black_win_rate = 100.0f * black_wins / current_game;
            float white_win_rate = 100.0f * white_wins / current_game;
            float draw_rate = 100.0f * draws / current_game;
            
            // Calculate learner (Black) win rate (excluding draws)
            int decided_games = black_wins + white_wins;
            float learner_win_rate = (decided_games > 0) ? 
                100.0f * black_wins / decided_games : 0.0f;
            
            std::cout << "[Updater] Game " << std::setw(6) << current_game << "/" << total_games
                      << " | Learner:" << std::setw(5) << std::setprecision(1) << std::fixed << learner_win_rate << "%"
                      << " | B:" << std::setw(5) << black_wins << " (" << std::setw(5) << std::setprecision(1) << black_win_rate << "%)"
                      << " W:" << std::setw(5) << white_wins << " (" << std::setw(5) << std::setprecision(1) << white_win_rate << "%)"
                      << " D:" << std::setw(4) << draws << " (" << std::setw(4) << std::setprecision(1) << draw_rate << "%)"
                      << " | LR:" << std::setw(6) << std::setprecision(4) << current_lr
                      << " | " << std::setw(4) << std::setprecision(1) << avg_moves << "m"
                      << " | " << std::setw(5) << std::setprecision(1) << games_per_sec << " g/s"
                      << " | Queue:" << result_queue.size()
                      << " | Wait:" << std::setw(4) << std::setprecision(1) << avg_wait_time << "ms"
                      << " Update:" << std::setw(4) << std::setprecision(1) << avg_update_time << "ms"
                      << " Total:" << std::setw(5) << std::setprecision(1) << avg_total_time << "ms"
                      << "\n";
            
            // Reset interval counters
            games_since_report = 0;
            total_wait_time_ms = 0;
            total_update_time_ms = 0;
        }
        
        // Save checkpoint
        if (current_game % config.save_interval == 0) {
            std::string checkpoint_path = config.save_path + "." + std::to_string(current_game);
            network.save(checkpoint_path);
            std::cout << "[Updater] Saved checkpoint: " << checkpoint_path << "\n";
        }
    }
    
    std::cout << "[Updater] Finished processing " << games_processed.load() << " games\n";
    
    int total_processed = games_processed.load();
    int decided_games = black_wins + white_wins;
    float learner_win_rate = (decided_games > 0) ? 
        100.0f * black_wins / decided_games : 0.0f;
    
    std::cout << "\n=== Final Statistics ===\n";
    std::cout << "  Learner (Black) win rate: " << std::setprecision(2) << std::fixed 
              << learner_win_rate << "% (" << black_wins << " wins / " << decided_games << " decided games)\n";
    std::cout << "  Black wins: " << black_wins << " (" << (100.0f * black_wins / total_processed) << "%)\n";
    std::cout << "  White wins: " << white_wins << " (" << (100.0f * white_wins / total_processed) << "%)\n";
    std::cout << "  Draws: " << draws << " (" << (100.0f * draws / total_processed) << "%)\n";
    std::cout << "  Average moves: " << (total_moves / total_processed) << "\n";
}

/**
 * 並列学習メイン関数
 */
void train_network_parallel(const TrainingConfig& config) {
    SharedNTupleNetwork network;
    
    // Load existing weights if specified
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
    
    // Automatically set save_interval
    int actual_save_interval = config.save_interval;
    if (config.save_interval == 1000) {
        actual_save_interval = std::max(100, config.num_games / 10);
        std::cout << "Auto-adjusting save interval to " << actual_save_interval << "\n";
    }
    
    TrainingConfig adjusted_config = config;
    adjusted_config.save_interval = actual_save_interval;
    
    std::cout << "\n=== Parallel N-tuple Network Training ===\n";
    std::cout << "Configuration:\n";
    std::cout << "  Games: " << config.num_games << "\n";
    std::cout << "  Worker threads: " << config.num_worker_threads << "\n";
    std::cout << "  Learning rate schedule: 0.1 -> 0.005 (inverse-square decay)\n";
    std::cout << "  Learner exploration rate (Black): " << config.exploration_rate << "\n";
    std::cout << "  Opponent exploration rate (White): " << config.opponent_exploration_rate;
    if (config.opponent_exploration_rate == 0.0f) {
        std::cout << " (greedy)";
    }
    std::cout << "\n";
    std::cout << "  Save interval: " << actual_save_interval << "\n";
    std::cout << "  Save path: " << config.save_path << "\n\n";
    
    // Create result queue
    GameResultQueue result_queue;
    std::atomic<int> games_completed(0);
    std::atomic<int> games_processed(0);
    
    auto start_time = std::chrono::steady_clock::now();
    
    // Start updater thread
    std::thread updater(updater_thread, std::ref(network), std::ref(result_queue), 
                        std::ref(games_processed), config.num_games, std::cref(adjusted_config));
    
    // Start worker threads
    std::vector<std::thread> workers;
    for (int i = 0; i < config.num_worker_threads; ++i) {
        workers.emplace_back(worker_thread, i, std::ref(network), std::ref(result_queue),
                             std::ref(games_completed), config.num_games, 
                             config.exploration_rate, config.opponent_exploration_rate);
    }
    
    // Wait for all workers to finish
    for (auto& worker : workers) {
        worker.join();
    }
    std::cout << "\nAll workers finished. Waiting for updater to process remaining results...\n";
    
    // Signal queue that no more results will be added
    result_queue.set_done();
    
    // Wait for updater to finish
    updater.join();
    
    // Final save
    network.save(config.save_path);
    
    auto end_time = std::chrono::steady_clock::now();
    auto total_elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        end_time - start_time).count();
    
    std::cout << "\n=== Training Complete ===\n";
    std::cout << "Total time: " << total_elapsed << " seconds\n";
    std::cout << "Games played: " << games_completed.load() << "\n";
    std::cout << "Games processed: " << games_processed.load() << "\n";
    std::cout << "Weights saved to: " << config.save_path << "\n";
}

/**
 * メイン関数
 */
int main(int argc, char* argv[]) {
    TrainingConfig config;
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--games" && i + 1 < argc) {
            config.num_games = std::stoi(argv[++i]);
        } else if (arg == "--epsilon" && i + 1 < argc) {
            config.exploration_rate = std::stof(argv[++i]);
        } else if (arg == "--opponent-epsilon" && i + 1 < argc) {
            config.opponent_exploration_rate = std::stof(argv[++i]);
        } else if (arg == "--threads" && i + 1 < argc) {
            config.num_worker_threads = std::stoi(argv[++i]);
        } else if (arg == "--save-interval" && i + 1 < argc) {
            config.save_interval = std::stoi(argv[++i]);
        } else if (arg == "--output" && i + 1 < argc) {
            config.save_path = argv[++i];
        } else if (arg == "--load" && i + 1 < argc) {
            config.load_path = argv[++i];
        } else if (arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [options]\n";
            std::cout << "Options:\n";
            std::cout << "  --games N              Number of training games (default: 10000)\n";
            std::cout << "  --epsilon EPS          Learner's exploration rate (default: 0.1)\n";
            std::cout << "  --opponent-epsilon EPS Opponent's exploration rate (default: 0.1, use 0.0 for greedy)\n";
            std::cout << "  --threads N            Number of worker threads (default: 4)\n";
            std::cout << "  --save-interval N      Save checkpoint every N games (default: auto)\n";
            std::cout << "  --output PATH          Output file path (default: ntuple_weights.bin)\n";
            std::cout << "  --load PATH            Load existing weights before training\n";
            std::cout << "  --help                 Show this help message\n";
            return 0;
        }
    }
    
    // Determine output directory: prefer environment variable TRAIN_OUTPUT_DIR,
    // otherwise default to /home/matsu-lab/terauchi (HDD directory requested by user).
    const char* env_out = std::getenv("TRAIN_OUTPUT_DIR");
    std::string default_dir = "/home/matsu-lab/terauchi";
    std::string out_dir = env_out ? std::string(env_out) : default_dir;

    // If user didn't provide --output, set default save path under out_dir
    if (config.save_path.empty() || config.save_path == "ntuple_weights.bin") {
        config.save_path = out_dir + "/ntuple_weights.bin";
    }

    // Setup log file: prefer TRAIN_LOG_DIR, else use the same out_dir
    const char* env_log = std::getenv("TRAIN_LOG_DIR");
    std::string log_dir = env_log ? std::string(env_log) : out_dir;
    std::string log_path = log_dir + "/train_parallel.log";

    static std::ofstream log_stream;
    log_stream.open(log_path, std::ios::out | std::ios::app);
    if (log_stream) {
        // Inform user on stdout before redirecting (so it appears in terminal)
        std::cout << "Redirecting logs to: " << log_path << "\n";
        std::cout.flush();
        // Redirect cout/cerr to log file so runtime logs are saved
        std::cout.rdbuf(log_stream.rdbuf());
        std::cerr.rdbuf(log_stream.rdbuf());
    } else {
        std::cerr << "Warning: failed to open log file: " << log_path << "\n";
    }

    train_network_parallel(config);
    return 0;
}
