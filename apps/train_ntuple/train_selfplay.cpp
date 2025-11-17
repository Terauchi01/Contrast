#include "contrast/game_state.hpp"
#include "contrast/rules.hpp"
#include "contrast/move_list.hpp"
#include "contrast_ai/ntuple.hpp"
#include <iostream>
#include <random>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <deque>
#include <algorithm>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <queue>
#include <memory>
#include <cstdlib>
#include <filesystem>

using namespace contrast;
using namespace contrast_ai;

/**
 * 学習の設定パラメータ
 */
struct TrainingConfig {
    int num_games = 100000;
    float learning_rate = 0.01f;
    float exploration_rate = 0.1f;
    int initial_training_games = 1000;     // 初期学習ゲーム数
    int swap_interval = 10000;             // Black/White入れ替え間隔
    int evaluation_window = 1000;          // 勝率評価ウィンドウ
    float promotion_threshold = 0.55f;     // 対戦相手昇格閾値
    int save_interval = 10000;
    int num_worker_threads = 7;
    std::string save_path = "ntuple_selfplay.bin";
    std::string load_path = "";
    bool log_role_swap = true;              // ロール入れ替え時のログ出力を行うか
};

/**
 * 1ゲームの結果を保持する構造体
 */
struct GameResult {
    std::vector<GameState> states;
    std::vector<Player> players;
    Player winner;
    int num_moves;
    bool learner_was_black;  // 学習プレイヤーがBlackだったか
};

/**
 * スレッドセーフなゲーム結果キュー
 */
class GameResultQueue {
private:
    std::queue<GameResult> queue_;
    mutable std::mutex mutex_;
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
            return false;
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
 * スレッドセーフなN-tupleネットワークラッパー
 */
class SharedNTupleNetwork {
private:
    NTupleNetwork network_;
    mutable std::mutex mutex_;
    
public:
    SharedNTupleNetwork() = default;
    
    float evaluate(const GameState& state) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return network_.evaluate(state);
    }
    
    void td_update(const GameState& state, float target, float learning_rate) {
        std::lock_guard<std::mutex> lock(mutex_);
        network_.td_update(state, target, learning_rate);
    }
    
    void save(const std::string& path) const {
        std::lock_guard<std::mutex> lock(mutex_);
        network_.save(path);
    }
    
    void load(const std::string& path) {
        std::lock_guard<std::mutex> lock(mutex_);
        network_.load(path);
    }
    
    // 現在のネットワークをコピーして返す
    NTupleNetwork copy() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return network_;
    }
    
    // 別のネットワークで置き換える
    void replace(const NTupleNetwork& other) {
        std::lock_guard<std::mutex> lock(mutex_);
        network_ = other;
    }
};

/**
 * ε-greedyで手を選択
 */
Move select_move_epsilon_greedy(
    const GameState& state,
    const NTupleNetwork& network,
    float epsilon,
    std::mt19937& rng
) {
    MoveList moves;
    Rules::legal_moves(state, moves);
    
    if (moves.empty()) {
        return Move{};
    }
    
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    if (dist(rng) < epsilon) {
        std::uniform_int_distribution<int> move_dist(0, static_cast<int>(moves.size - 1));
        return moves[move_dist(rng)];
    }
    
    Move best_move = moves[0];
    float best_value = -std::numeric_limits<float>::infinity();
    
    for (const auto& move : moves) {
        GameState next_state = state;
        next_state.apply_move(move);
        float value = -network.evaluate(next_state);
        
        if (value > best_value) {
            best_value = value;
            best_move = move;
        }
    }
    
    return best_move;
}

/**
 * ε-greedyで手を選択（SharedNTupleNetwork版）
 */
Move select_move_epsilon_greedy_shared(
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
    
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    if (dist(rng) < epsilon) {
        std::uniform_int_distribution<int> move_dist(0, static_cast<int>(moves.size - 1));
        return moves[move_dist(rng)];
    }
    
    Move best_move = moves[0];
    float best_value = -std::numeric_limits<float>::infinity();
    
    for (const auto& move : moves) {
        GameState next_state = state;
        next_state.apply_move(move);
        // SharedNTupleNetworkのevaluateを呼ぶ（スレッドセーフ）
        float value = -network.evaluate(next_state);
        
        if (value > best_value) {
            best_value = value;
            best_move = move;
        }
    }
    
    return best_move;
}

/**
 * 自己対戦で1ゲームをプレイ（SharedNTupleNetwork版）
 * learner: 学習プレイヤー（SharedNTupleNetwork、読み取り専用）
 * opponent: 対戦相手（固定、定期的に更新）
 * learner_is_black: 学習プレイヤーがBlackかどうか
 */
GameResult play_selfplay_game_shared(
    const SharedNTupleNetwork& learner,
    const NTupleNetwork& opponent,
    bool learner_is_black,
    float learner_epsilon,
    std::mt19937& rng,
    Player start_player
) {
    GameResult result;
    result.learner_was_black = learner_is_black;
    GameState state;
    state.reset();
    // Set starting player based on requested start_player (parity control)
    state.to_move_ = start_player;
    
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
        
        // 現在のプレイヤーが学習者かどうか
        bool current_is_learner = (state.current_player() == Player::Black) == learner_is_black;
        
        Move move;
        if (current_is_learner) {
            // 学習プレイヤー：SharedNetworkから直接評価（スレッドセーフ）
            move = select_move_epsilon_greedy_shared(state, learner, learner_epsilon, rng);
        } else {
            // 対戦相手：greedy（探索なし）
            move = select_move_epsilon_greedy(state, opponent, 0.0f, rng);
        }
        
        state.apply_move(move);
        move_count++;
    }
    
    result.winner = Player::None;
    result.num_moves = move_count;
    return result;
}

/**
 * 自己対戦で1ゲームをプレイ
 * learner: 学習プレイヤー（常に更新される）
 * opponent: 対戦相手（固定、定期的に更新）
 * learner_is_black: 学習プレイヤーがBlackかどうか
 */
GameResult play_selfplay_game(
    const NTupleNetwork& learner,
    const NTupleNetwork& opponent,
    bool learner_is_black,
    float learner_epsilon,
    std::mt19937& rng,
    Player start_player
) {
    GameResult result;
    result.learner_was_black = learner_is_black;
    GameState state;
    state.reset();
    // Set starting player based on requested start_player (parity control)
    state.to_move_ = start_player;
    
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
        
        // 現在のプレイヤーが学習者かどうか
        bool current_is_learner = (state.current_player() == Player::Black) == learner_is_black;
        
        Move move;
        if (current_is_learner) {
            // 学習プレイヤー：探索あり
            move = select_move_epsilon_greedy(state, learner, learner_epsilon, rng);
        } else {
            // 対戦相手：greedy（探索なし）
            move = select_move_epsilon_greedy(state, opponent, 0.0f, rng);
        }
        
        state.apply_move(move);
        move_count++;
    }
    
    result.winner = Player::None;
    result.num_moves = move_count;
    return result;
}

/**
 * ワーカースレッド：ゲームをプレイして結果をキューに追加
 */
void worker_thread(
    int worker_id,
    const SharedNTupleNetwork& learner_network,
    std::shared_ptr<NTupleNetwork>& opponent_network_ptr,
    std::atomic<bool>& learner_is_black,
    GameResultQueue& result_queue,
    std::atomic<int>& games_completed,
    int target_games,
    float learner_epsilon
) {
    std::mt19937 rng(std::random_device{}() + worker_id);
    
    std::cout << "[Worker " << worker_id << "] Started" << std::endl;
    
    while (true) {
        int game_num = games_completed.fetch_add(1);
        if (game_num >= target_games) {
            break;
        }
        
    // 現在の対戦相手とlearner色を取得
    // atomic_load により shared_ptr を安全に取得
    std::shared_ptr<NTupleNetwork> opponent_ptr = std::atomic_load(&opponent_network_ptr);
    bool is_black = learner_is_black.load();
    // Determine starting player by game parity: odd -> White first, even -> Black first
    int game_no = game_num + 1; // make 1-based
    Player start_player = (game_no % 2 == 1) ? Player::White : Player::Black;
        
    // ゲームプレイ（learner_networkは読み取り専用でSharedから直接評価）
    GameResult result = play_selfplay_game_shared(learner_network, *opponent_ptr, is_black, learner_epsilon, rng, start_player);
        
        result_queue.push(std::move(result));
    }
    
    std::cout << "[Worker " << worker_id << "] Finished" << std::endl;
}

/**
 * 更新スレッド：キューからゲーム結果を取り出して重みを更新
 */
void updater_thread(
    SharedNTupleNetwork& learner_network,
    std::shared_ptr<NTupleNetwork>& opponent_network_ptr,
    std::atomic<bool>& learner_is_black,
    GameResultQueue& result_queue,
    std::atomic<int>& games_processed,
    int total_games,
    const TrainingConfig& config
) {
    std::cout << "[Updater] Started" << std::endl;
    
    int learner_wins = 0;
    int opponent_wins = 0;
    int draws = 0;
    float total_moves = 0;
    
    // 直近1000ゲームの勝率追跡
    std::deque<bool> recent_wins;  // true = learner win
    int recent_learner_wins = 0;
    
    // 対戦相手ネットワーク（最初は学習者のコピー）
    auto initial_opponent = std::make_shared<NTupleNetwork>(learner_network.copy());
    std::atomic_store(&opponent_network_ptr, initial_opponent);
    
    auto start_time = std::chrono::steady_clock::now();
    int last_swap_game = 0;
    int games_since_last_promotion = 0;
    
    while (true) {
        GameResult result;
        if (!result_queue.pop(result)) {
            break;
        }
        
        int current_game = games_processed.fetch_add(1) + 1;
        
        // 学習率の減衰（inverse-square decay）
        float current_lr = 0.1f / (1.0f + current_game / 10000.0f);
        current_lr = std::max(current_lr, 0.005f);
        
        // TD学習
        for (size_t i = 0; i < result.states.size(); ++i) {
            const auto& state = result.states[i];
            Player player = result.players[i];
            
            // 学習プレイヤーの手番のみ学習
            bool state_is_learner = (player == Player::Black) == result.learner_was_black;
            if (!state_is_learner) continue;
            
            float target = 0.0f;
            if (result.winner == player) {
                target = 1.0f;
            } else if (result.winner != Player::None && result.winner != player) {
                target = -1.0f;
            }
            
            learner_network.td_update(state, target, current_lr);
        }
        
        // 統計更新
        bool learner_won = (result.winner == Player::Black && result.learner_was_black) ||
                          (result.winner == Player::White && !result.learner_was_black);
        
        if (learner_won) {
            learner_wins++;
        } else if (result.winner == Player::None) {
            draws++;
        } else {
            opponent_wins++;
        }
        total_moves += result.num_moves;
        
        // 直近1000ゲームの勝率追跡
        recent_wins.push_back(learner_won);
        if (learner_won) recent_learner_wins++;
        
        if (recent_wins.size() > config.evaluation_window) {
            if (recent_wins.front()) recent_learner_wins--;
            recent_wins.pop_front();
        }
        
        games_since_last_promotion++;
        
        // 進捗レポート
        if (current_game % 100 == 0) {
            auto current_time = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                current_time - start_time).count();
            
            int decided_games = learner_wins + opponent_wins;
            float learner_win_rate = (decided_games > 0) ? 
                100.0f * learner_wins / decided_games : 0.0f;
            
            float recent_win_rate = (recent_wins.size() > 0) ?
                100.0f * recent_learner_wins / recent_wins.size() : 0.0f;
            
            float avg_moves = total_moves / current_game;
            float games_per_sec = current_game / static_cast<float>(elapsed + 1);
            
            std::cout << "[Updater] Game " << std::setw(6) << current_game << "/" << total_games
                      << " | Overall:" << std::setw(5) << std::setprecision(1) << std::fixed << learner_win_rate << "%"
                      << " | Recent(" << recent_wins.size() << "):" << std::setw(5) << std::setprecision(1) << recent_win_rate << "%"
                      << " | L:" << std::setw(5) << learner_wins 
                      << " O:" << std::setw(5) << opponent_wins
                      << " D:" << std::setw(4) << draws
                      << " | LR:" << std::setw(6) << std::setprecision(4) << current_lr
                      << " | " << std::setw(4) << std::setprecision(1) << avg_moves << "m"
                      << " | " << std::setw(5) << std::setprecision(1) << games_per_sec << " g/s"
                      << " | Learner:" << (learner_is_black.load() ? "Black" : "White")
                      << std::endl;  // \n から std::endl に変更（強制フラッシュ）
        }
        
        // 初期学習完了後の処理
        if (current_game > config.initial_training_games) {
            // Black/White入れ替え
            if (current_game - last_swap_game >= config.swap_interval) {
                learner_is_black.store(!learner_is_black.load());
                last_swap_game = current_game;
                    if (config.log_role_swap) {
                        std::cout << "[System] Swapped roles - Learner is now " 
                                  << (learner_is_black.load() ? "Black" : "White") << std::endl;
                    }
            }
            
            // 対戦相手の昇格チェック
            if (recent_wins.size() >= config.evaluation_window && 
                games_since_last_promotion >= config.evaluation_window) {
                
                float recent_rate = static_cast<float>(recent_learner_wins) / recent_wins.size();
                
                if (recent_rate >= config.promotion_threshold) {
                    // 対戦相手を現在の学習者に置き換える
                    auto new_opponent = std::make_shared<NTupleNetwork>(learner_network.copy());
                    // 古い対戦相手を取得してから差し替え
                    auto prev_opponent = std::atomic_load(&opponent_network_ptr);
                    std::atomic_store(&opponent_network_ptr, new_opponent);

                    // タイムスタンプを取得
                    auto now = std::chrono::system_clock::now();
                    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
                    char timebuf[64];
                    if (std::strftime(timebuf, sizeof(timebuf), "%F %T", std::localtime(&now_c))) {
                        std::cout << "[System] [" << timebuf << "] ";
                    }

                    // 詳細ログ出力（ゲーム番号、勝率、閾値、旧/新ポインタ）
                    std::cout << "*** OPPONENT PROMOTED at game " << current_game << " ***" << std::endl;
                    std::cout << "[System]     Recent win rate: " << std::setprecision(1) << std::fixed 
                              << (recent_rate * 100.0f) << "% (threshold: " 
                              << (config.promotion_threshold * 100.0f) << "%)" << std::endl;
                    std::cout << "[System]     Previous opponent ptr: " << prev_opponent.get()
                              << "  -> New opponent ptr: " << new_opponent.get() << std::endl;
                    std::cout << "[System]     Opponent updated to current learner" << std::endl;

                    // 統計リセット
                    recent_wins.clear();
                    recent_learner_wins = 0;
                    games_since_last_promotion = 0;
                }
            }
        }
        
        // チェックポイント保存
        if (current_game % config.save_interval == 0) {
            // 保存先ディレクトリの決定: 環境変数 TRAIN_SAVE_DIR があればそれを優先
            std::filesystem::path save_dir;
            if (const char* env_p = std::getenv("TRAIN_SAVE_DIR")) {
                save_dir = std::string(env_p);
            } else {
                // config.save_path にディレクトリが含まれていればそれを使い、含まれていなければデフォルトの bin ディレクトリを使う
                std::filesystem::path configured_path(config.save_path);
                if (!configured_path.has_parent_path() || configured_path.parent_path().string().empty()) {
                    save_dir = std::filesystem::path("/home/matsu-lab/terauchi/Contrast/bin");
                } else {
                    save_dir = configured_path.parent_path();
                }
            }

            // ディレクトリ存在を保証
            std::error_code ec;
            std::filesystem::create_directories(save_dir, ec);
            if (ec) {
                std::cerr << "[Updater] Warning: failed to create save directory: " << save_dir << " -> " << ec.message() << std::endl;
            }

            // ファイル名を組み立て
            std::string base_name = std::filesystem::path(config.save_path).filename().string();
            std::string checkpoint_name = base_name + "." + std::to_string(current_game);
            std::filesystem::path checkpoint_path = save_dir / checkpoint_name;

            learner_network.save(checkpoint_path.string());
            std::cout << "[Updater] Saved checkpoint: " << checkpoint_path.string() << std::endl;
        }
    }
    
    std::cout << "[Updater] Finished processing " << games_processed.load() << " games\n";
    
    int total_processed = games_processed.load();
    int decided_games = learner_wins + opponent_wins;
    float learner_win_rate = (decided_games > 0) ? 
        100.0f * learner_wins / decided_games : 0.0f;
    
    std::cout << "\n=== Final Statistics ===\n";
    std::cout << "  Learner overall win rate: " << std::setprecision(2) << std::fixed 
              << learner_win_rate << "% (" << learner_wins << " wins / " << decided_games << " decided games)\n";
    std::cout << "  Learner wins: " << learner_wins << " (" << (100.0f * learner_wins / total_processed) << "%)\n";
    std::cout << "  Opponent wins: " << opponent_wins << " (" << (100.0f * opponent_wins / total_processed) << "%)\n";
    std::cout << "  Draws: " << draws << " (" << (100.0f * draws / total_processed) << "%)\n";
    std::cout << "  Average moves: " << (total_moves / total_processed) << "\n";
}

/**
 * 自己対戦学習メイン関数
 */
void train_network_selfplay(const TrainingConfig& config) {
    SharedNTupleNetwork learner_network;
    
    if (!config.load_path.empty()) {
        std::cout << "Loading existing weights from: " << config.load_path << "\n";
        try {
            learner_network.load(config.load_path);
            std::cout << "Weights loaded successfully!\n";
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to load weights: " << e.what() << "\n";
            std::cerr << "Starting with random weights instead.\n";
        }
    }
    
    std::cout << "\n=== Self-Play N-tuple Network Training ===\n";
    std::cout << "Configuration:\n";
    std::cout << "  Total games: " << config.num_games << "\n";
    std::cout << "  Initial training games: " << config.initial_training_games << "\n";
    std::cout << "  Worker threads: " << config.num_worker_threads << "\n";
    std::cout << "  Learning rate schedule: 0.1 -> 0.005 (inverse-square decay)\n";
    std::cout << "  Learner exploration rate: " << config.exploration_rate << "\n";
    std::cout << "  Opponent exploration rate: 0.0 (greedy)\n";
    std::cout << "  Role swap interval: " << config.swap_interval << " games\n";
    std::cout << "  Evaluation window: " << config.evaluation_window << " games\n";
    std::cout << "  Promotion threshold: " << (config.promotion_threshold * 100.0f) << "%\n";
    std::cout << "  Save interval: " << config.save_interval << "\n";
    std::cout << "  Save path: " << config.save_path << "\n\n";
    
    GameResultQueue result_queue;
    std::atomic<int> games_completed(0);
    std::atomic<int> games_processed(0);
    std::atomic<bool> learner_is_black(true);

    // 対戦相手ネットワークの共有ポインタ（updaterスレッドで管理）
    // std::atomic_load / std::atomic_store を使って安全に公開する
    std::shared_ptr<NTupleNetwork> opponent_network_ptr;
    
    auto start_time = std::chrono::steady_clock::now();
    
    // 更新スレッド開始
    std::thread updater(updater_thread, std::ref(learner_network), 
                        std::ref(opponent_network_ptr), std::ref(learner_is_black),
                        std::ref(result_queue), std::ref(games_processed), 
                        config.num_games, std::cref(config));
    
    // 対戦相手が初期化されるまで待つ
    while (std::atomic_load(&opponent_network_ptr) == nullptr) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // ワーカースレッド開始
    std::vector<std::thread> workers;
    for (int i = 0; i < config.num_worker_threads; ++i) {
        workers.emplace_back(worker_thread, i, std::cref(learner_network), 
                             std::ref(opponent_network_ptr), std::ref(learner_is_black),
                             std::ref(result_queue), std::ref(games_completed), 
                             config.num_games, config.exploration_rate);
    }
    
    // ワーカー完了待機
    for (auto& worker : workers) {
        worker.join();
    }
    std::cout << "\nAll workers finished. Waiting for updater to process remaining results...\n";
    
    result_queue.set_done();
    updater.join();
    
    // 最終保存
    learner_network.save(config.save_path);
    
    auto end_time = std::chrono::steady_clock::now();
    auto total_elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        end_time - start_time).count();
    
    std::cout << "\n=== Training Complete ===\n";
    std::cout << "Total time: " << total_elapsed << " seconds\n";
    std::cout << "Games played: " << games_completed.load() << "\n";
    std::cout << "Games processed: " << games_processed.load() << "\n";
    std::cout << "Weights saved to: " << config.save_path << "\n";
}

int main(int argc, char* argv[]) {
    TrainingConfig config;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--games" && i + 1 < argc) {
            config.num_games = std::stoi(argv[++i]);
        } else if (arg == "--epsilon" && i + 1 < argc) {
            config.exploration_rate = std::stof(argv[++i]);
        } else if (arg == "--threads" && i + 1 < argc) {
            config.num_worker_threads = std::stoi(argv[++i]);
        } else if (arg == "--initial" && i + 1 < argc) {
            config.initial_training_games = std::stoi(argv[++i]);
        } else if (arg == "--swap-interval" && i + 1 < argc) {
            config.swap_interval = std::stoi(argv[++i]);
        } else if (arg == "--eval-window" && i + 1 < argc) {
            config.evaluation_window = std::stoi(argv[++i]);
        } else if (arg == "--promotion-threshold" && i + 1 < argc) {
            config.promotion_threshold = std::stof(argv[++i]);
        } else if (arg == "--no-swap-log") {
            config.log_role_swap = false;
        } else if (arg == "--save-interval" && i + 1 < argc) {
            config.save_interval = std::stoi(argv[++i]);
        } else if (arg == "--output" && i + 1 < argc) {
            config.save_path = argv[++i];
        } else if (arg == "--load" && i + 1 < argc) {
            config.load_path = argv[++i];
        } else if (arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [options]\n";
            std::cout << "Options:\n";
            std::cout << "  --games N                Number of training games (default: 100000)\n";
            std::cout << "  --epsilon EPS            Learner's exploration rate (default: 0.1)\n";
            std::cout << "  --threads N              Number of worker threads (default: 7)\n";
            std::cout << "  --initial N              Initial training games (default: 1000)\n";
            std::cout << "  --swap-interval N        Black/White swap interval (default: 10000)\n";
            std::cout << "  --eval-window N          Evaluation window size (default: 1000)\n";
            std::cout << "  --promotion-threshold T  Win rate threshold for promotion (default: 0.55)\n";
            std::cout << "  --save-interval N        Save checkpoint every N games (default: 10000)\n";
            std::cout << "  --no-swap-log            Disable 'Swapped roles' log messages\n";
            std::cout << "  --output PATH            Output file path (default: ntuple_selfplay.bin)\n";
            std::cout << "  --load PATH              Load existing weights before training\n";
            std::cout << "  --help                   Show this help message\n";
            return 0;
        }
    }
    
    train_network_selfplay(config);
    return 0;
}
