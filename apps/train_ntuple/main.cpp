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
#include <set>

using namespace contrast;
using namespace contrast_ai;

/**
 * N-tupleパターンを視覚的に表示
 */
void print_ntuple_pattern(const NTuple& pattern, int index) {
    std::set<int> cells;
    for (size_t i = 0; i < pattern.num_cells; ++i) {
        cells.insert(pattern.cell_indices[i]);
    }
    
    std::cout << "  Pattern #" << std::setw(2) << index << " (" << pattern.num_cells << " cells): [";
    for (size_t i = 0; i < pattern.num_cells; ++i) {
        if (i > 0) std::cout << ",";
        std::cout << std::setw(2) << pattern.cell_indices[i];
    }
    std::cout << "]\n";
    
    // 盤面表示（5x5）
    for (int y = 0; y < 5; ++y) {
        std::cout << "    ";
        for (int x = 0; x < 5; ++x) {
            int cell = y * 5 + x;
            if (cells.count(cell)) {
                std::cout << "■ ";
            } else {
                std::cout << "□ ";
            }
        }
        std::cout << "\n";
    }
}

/**
 * 学習の設定パラメータ
 * 
 * コマンドライン引数で上書き可能な学習設定を保持
 */
struct TrainingConfig {
    int num_games = 10000;              // 総学習ゲーム数
    long long num_turns = 0;            // 総学習ターン数（TD更新回数, 0=未使用）
    float learning_rate = 0.01f;        // 学習率（実際は動的に変化）
    float discount_factor = 0.9f;       // 割引率（現在は未使用、TD(0)のため）
    float exploration_rate = 0.1f;      // ε-greedy の探索率
    int save_interval = 1000;           // チェックポイント保存間隔
    std::string save_path = "ntuple_weights.bin";  // 保存先パス
    std::string load_path = "";         // 事前学習済み重みの読み込み元（オプション）
    std::string opponent = "self";      // 対戦相手: "self"=前回の自分, "greedy"=Greedy, "rulebased"=RuleBased
};

/**
 * ε-greedy法による手の選択
 * 
 * 確率εでランダムな手を選び（探索）、確率(1-ε)で評価値最大の手を選ぶ（活用）
 * Negamaxフレームワークで評価：相手の評価値 = -自分の評価値
 * 
 * @param state 現在の盤面
 * @param network 評価関数（NTupleネットワーク）
 * @param epsilon 探索率（0.0=常に最良手, 1.0=常にランダム）
 * @param rng 乱数生成器
 * @return 選択された手
 */
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
 * 1ゲームの結果を保持する構造体
 * 
 * TD学習のため、ゲーム中の全状態と各手番のプレイヤー、最終勝者を記録
 */
struct GameResult {
    std::vector<GameState> states;      // ゲーム中の全盤面状態
    std::vector<Player> players;        // 各状態でのプレイヤー（Black/White）
    Player winner;                      // 勝者（Black/White/None=引き分け）
    int num_moves;                      // 総手数
};

/**
 * 学習用に1ゲームをプレイして軌跡を記録
 * 
 * 対戦相手の種類に応じて異なるモード：
 * 1. セルフプレイ（previous_network指定時）: 現在のネットワーク vs 前回のチェックポイント
 * 2. vs Greedy（greedy_opponent指定時）: NTuple vs Greedy
 * 3. vs RuleBased（rulebased_opponent指定時）: NTuple vs RuleBased
 * 4. 純粋セルフプレイ（すべてnull）: NTuple vs NTuple（同じネットワーク）
 * 
 * swap_colorsでNTupleの担当色を変更可能（学習バランスのため）
 * 
 * @param network 学習中のNTupleネットワーク
 * @param config 学習設定
 * @param greedy_opponent Greedyポリシー（vs Greedyモード時のみ）
 * @param rulebased_opponent RuleBasedポリシー（vs RuleBasedモード時のみ）
 * @param previous_network 前回のチェックポイント（セルフプレイ改良版時のみ）
 * @param rng 乱数生成器
 * @param swap_colors true=NTupleがWhite, false=NTupleがBlack
 * @return ゲーム結果（状態軌跡、勝者、手数）
 */
GameResult play_training_game(
    NTupleNetwork& network,
    const TrainingConfig& config,
    GreedyPolicy* greedy_opponent,
    RuleBasedPolicy* rulebased_opponent,
    NTupleNetwork* previous_network,
    std::mt19937& rng,
    bool swap_colors = false
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
        
        // Select and apply move
        Move move;
        if (greedy_opponent != nullptr) {
            // NTuple vs Greedy
            bool ntuple_turn = swap_colors ? (state.current_player() == Player::White) 
                                          : (state.current_player() == Player::Black);
            if (ntuple_turn) {
                move = select_move_epsilon_greedy(state, network, config.exploration_rate, rng);
            } else {
                move = greedy_opponent->pick(state);
            }
        } else if (rulebased_opponent != nullptr) {
            // NTuple vs RuleBased
            bool ntuple_turn = swap_colors ? (state.current_player() == Player::White) 
                                          : (state.current_player() == Player::Black);
            if (ntuple_turn) {
                move = select_move_epsilon_greedy(state, network, config.exploration_rate, rng);
            } else {
                move = rulebased_opponent->pick(state);
            }
        } else if (previous_network != nullptr) {
            // 新しいネットワーク vs 前回のネットワーク
            bool current_turn = swap_colors ? (state.current_player() == Player::White) 
                                           : (state.current_player() == Player::Black);
            if (current_turn) {
                // 現在学習中のネットワーク
                move = select_move_epsilon_greedy(state, network, config.exploration_rate, rng);
            } else {
                // 前回のチェックポイント（探索なし）
                move = select_move_epsilon_greedy(state, *previous_network, 0.0f, rng);
            }
        } else {
            // Pure self-play: 常にNTupleネットワークを使用
            move = select_move_epsilon_greedy(state, network, config.exploration_rate, rng);
        }
        
        state.apply_move(move);
        move_count++;
    }
    
    // Max moves reached - draw
    result.winner = Player::None;
    result.num_moves = move_count;
    return result;
}

/**
 * TD(0)学習：ゲーム軌跡から重みを更新
 * 
 * 最終結果のみを使用（勝ち=+1, 負け=-1, 引き分け=0）
 * 各状態で target - current_eval の誤差を計算し、重みを更新
 * 
 * vs Greedy や前回ネットワークとの対戦時は、NTupleが指した手のみ更新
 * （相手の手は学習対象外）
 * 
 * @param network 更新対象のNTupleネットワーク
 * @param result ゲーム結果（状態軌跡と勝者）
 * @param current_learning_rate 現在の学習率
 * @param is_vs_greedy true=vs Greedyまたはvs前回ネットワーク（NTupleの手のみ更新）
 * @param swap_colors NTupleの担当色（どの手を更新対象とするか判定に使用）
 * @return {black_updates, white_updates} 各プレイヤーの更新回数
 */
std::pair<int, int> td_learn_from_game(
    NTupleNetwork& network,
    const GameResult& result,
    float current_learning_rate,
    bool is_vs_greedy,
    bool swap_colors = false,
    int max_updates = -1
) {
    if (result.states.empty()) return {0, 0};
    
    // デバッグ用：更新された手数をカウント
    int black_updates = 0;
    int white_updates = 0;
    
    // Backward updates from end to start
    // For each state, we assign reward based on final outcome from that player's perspective
    for (int i = result.states.size() - 1; i >= 0; --i) {
        if (max_updates >= 0 && (black_updates + white_updates) >= max_updates) {
            break;
        }
        const auto& state = result.states[i];
        Player player = result.players[i];
        
        // Determine which player is controlled by NTuple
        Player ntuple_player = swap_colors ? Player::White : Player::Black;
        
        // If playing vs Greedy or alternating colors, only update NTuple's moves
        if (is_vs_greedy && player != ntuple_player) {
            continue;
        }
        
        // デバッグ：どのプレイヤーの手を更新しているかカウント
        if (player == Player::Black) black_updates++;
        else white_updates++;
        
        // Determine target value based on final outcome
        float target = 0.0f;
        if (result.winner == player) {
            target = 1.0f;  // This player won
        } else if (result.winner != Player::None && result.winner != player) {
            target = -1.0f; // This player lost
        }
        // else draw = 0.0f
        
        // Update this state's value toward the target
        network.td_update(state, target, current_learning_rate);
    }
    
    return {black_updates, white_updates};
}

/**
 * 学習率スケジュール関数（逆二乗減衰）
 * 
 * 序盤は急速に減衰、中盤以降は緩やかに減衰する学習率を計算
 * これにより、初期の大きな更新で粗い最適化を行い、後半の小さな更新で微調整を行う
 * 
 * 使用する関数: lr = lr_min + (lr_max - lr_min) / (1 + k * progress^2)
 * 
 * 減衰の特性：
 * - progress = 0.0 (開始時) -> lr = lr_max (0.1)
 * - progress = 0.1 (10%時点) -> lr ≈ 0.085 (素早く減衰)
 * - progress = 0.5 (50%時点) -> lr ≈ 0.022 (中間値)
 * - progress = 1.0 (終了時) -> lr ≈ lr_min (0.005)
 * 
 * @param current_game 現在のゲーム番号
 * @param total_games 総ゲーム数
 * @param lr_max 最大学習率（初期値）
 * @param lr_min 最小学習率（終了時の値）
 * @return 現在の学習率
 */
float calculate_learning_rate(long long current_step, long long total_steps, float lr_max = 0.1f, float lr_min = 0.005f) {
    if (total_steps <= 1) {
        return lr_max;
    }

    double progress = static_cast<double>(current_step - 1) / static_cast<double>(total_steps - 1);
    if (progress < 0.0) progress = 0.0;
    if (progress > 1.0) progress = 1.0;
    
    // 逆数ベースの減衰（序盤急速、後半緩やか）
    // k=19 で、progress=0.5のとき約lr=0.025（中間値）になる
    float k = 19.0f;
    float lr = lr_min + (lr_max - lr_min) / static_cast<float>(1.0 + k * progress * progress);
    
    return lr;
}

/**
 * 学習時の統計情報を保持する構造体
 */
struct TrainingStats {
    int black_wins = 0;
    int white_wins = 0;
    int draws = 0;
    float total_moves = 0;
    int ntuple_wins = 0;
    int ntuple_losses = 0;
    int ntuple_draws = 0;
    int recent_black_updates = 0;
    int recent_white_updates = 0;
    int recent_games_count = 0;
    long long total_updates = 0;
    int games_played = 0;
    std::deque<bool> recent_wins;
};

/**
 * 学習時の対戦相手情報を保持する構造体
 */
struct OpponentState {
    GreedyPolicy* greedy = nullptr;
    RuleBasedPolicy* rulebased = nullptr;
    NTupleNetwork* previous = nullptr;
    std::string current_type;
    bool is_vs_greedy = false;
    bool is_vs_rulebased = false;
    bool is_vs_previous = false;
    
    ~OpponentState() {
        delete greedy;
        delete rulebased;
        delete previous;
    }
};

/**
 * ネットワークを初期化し、既存重みがあればロードする
 */
void initialize_network(NTupleNetwork& network, const std::string& load_path) {
    if (load_path.empty()) return;
    
    std::cout << "Loading existing weights from: " << load_path << "\n";
    try {
        network.load(load_path);
        std::cout << "Weights loaded successfully!\n";
    } catch (const std::exception& e) {
        std::cerr << "Warning: Failed to load weights: " << e.what() << "\n";
        std::cerr << "Starting with random weights instead.\n";
    }
}

/**
 * save_intervalを自動調整（総ゲーム数の1/10）
 */
int adjust_save_interval(const TrainingConfig& config) {
    if (config.save_interval != 1000) {  // デフォルト値以外なら変更しない
        return config.save_interval;
    }
    int adjusted = std::max(100, config.num_games / 10);
    std::cout << "Auto-adjusting save interval to " << adjusted 
              << " (1/10th of total games)\n";
    return adjusted;
}

/**
 * 対戦相手を初期化
 */
void initialize_opponent(OpponentState& opp, const std::string& opponent_type) {
    opp.current_type = opponent_type;
    opp.is_vs_greedy = (opponent_type == "greedy");
    opp.is_vs_rulebased = (opponent_type == "rulebased");
    opp.is_vs_previous = false;  // 常にリアルタイムセルフプレイ
    
    if (opp.is_vs_greedy) {
        opp.greedy = new GreedyPolicy();
        std::cout << "Training against Greedy opponent\n";
    } else if (opp.is_vs_rulebased) {
        opp.rulebased = new RuleBasedPolicy();
        std::cout << "Training against RuleBased opponent\n";
    } else {
        std::cout << "Training with real-time self-play (same network for both sides)\n";
    }
}

/**
 * 学習設定を表示
 */
void print_training_config(const TrainingConfig& config, int actual_save_interval) {
    std::cout << "\n=== N-tuple Network Training ===\n";
    std::cout << "\nConfiguration:\n";
    std::cout << "  Opponent: " << config.opponent << "\n";
    if (config.num_turns > 0) {
        std::cout << "  Turns: " << config.num_turns << " (TD updates)\n";
        std::cout << "  Games: " << config.num_games << " (cap)\n";
    } else {
        std::cout << "  Games: " << config.num_games << "\n";
    }
    std::cout << "  Initial learning rate: " << config.learning_rate << "\n";
    std::cout << "  Learning rate schedule: 0.1 -> 0.005 (inverse-square decay)\n";
    std::cout << "    - Fast decay in early games\n";
    std::cout << "    - Gradual decay in later games\n";
    std::cout << "  Discount factor: " << config.discount_factor << "\n";
    std::cout << "  Exploration rate: " << config.exploration_rate << "\n";
    std::cout << "  Save interval: " << actual_save_interval << "\n";
    std::cout << "  Save path: " << config.save_path << "\n\n";
}

/**
 * 対戦相手を切り替える
 */
void switch_opponent(OpponentState& opp, const NTupleNetwork& network, int game, float win_rate) {
    std::cout << "\n=== ";
    if (game == 1000) {
        std::cout << "INITIAL TRAINING COMPLETE";
    } else {
        std::cout << "WIN RATE THRESHOLD REACHED";
    }
    std::cout << " ===\n";
    std::cout << "Game: " << game << " | Recent 1000 games win rate: " 
              << (win_rate * 100.0f) << "%\n";
    std::cout << "Switching opponent from " << opp.current_type << " to ";
    
    if (opp.current_type == "greedy") {
        opp.current_type = "rulebased";
        delete opp.greedy;
        opp.greedy = nullptr;
        opp.rulebased = new RuleBasedPolicy();
        opp.is_vs_greedy = false;
        opp.is_vs_rulebased = true;
        opp.is_vs_previous = false;
        std::cout << "rulebased\n";
    } else if (opp.current_type == "rulebased") {
        opp.current_type = "self";
        delete opp.rulebased;
        opp.rulebased = nullptr;
        opp.is_vs_greedy = false;
        opp.is_vs_rulebased = false;
        opp.is_vs_previous = false;
        std::cout << "self (real-time self-play)\n";
    } else {
        std::cout << "self (already in real-time self-play mode)\n";
    }
    
    std::cout << "===================================\n\n";
}

/**
 * 対戦相手を切り替えるべきかチェック
 */
bool should_switch_opponent(const TrainingStats& stats, int game) {
    const int WIN_RATE_WINDOW = 1000;
    const float WIN_RATE_THRESHOLD = 0.55f;
    
    if (stats.recent_wins.size() < WIN_RATE_WINDOW) {
        return false;
    }
    
    int recent_win_count = std::count(stats.recent_wins.begin(), stats.recent_wins.end(), true);
    float recent_win_rate = static_cast<float>(recent_win_count) / WIN_RATE_WINDOW;
    
    return (game == WIN_RATE_WINDOW) || (recent_win_rate > WIN_RATE_THRESHOLD);
}

/**
 * 進捗を表示
 */
void print_progress(
    const TrainingConfig& config,
    const TrainingStats& stats,
    const OpponentState& opp,
    int game,
    bool swap_colors,
    std::chrono::steady_clock::time_point start_time
) {
    auto current_time = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        current_time - start_time).count();
    
    float avg_moves = stats.total_moves / game;
    float games_per_sec = game / static_cast<float>(elapsed + 1);
    
    // 学習率計算
    long long total_steps = (config.num_turns > 0) ? config.num_turns : static_cast<long long>(config.num_games);
    long long current_step = (config.num_turns > 0) ? std::min(config.num_turns, stats.total_updates + 1) : static_cast<long long>(game);
    float display_lr = calculate_learning_rate(current_step, std::max(1LL, total_steps));
    
    float ntuple_win_rate = 100.0f * stats.ntuple_wins / game;
    
    // 直近1000試合の勝率を計算
    float recent_1000_win_rate = 0.0f;
    if (!stats.recent_wins.empty()) {
        int recent_win_count = std::count(stats.recent_wins.begin(), stats.recent_wins.end(), true);
        recent_1000_win_rate = 100.0f * recent_win_count / stats.recent_wins.size();
    }
    
    std::cout << "Game " << std::setw(6) << game << "/" << config.num_games;
    if (config.num_turns > 0) {
        std::cout << " | Turns " << stats.total_updates << "/" << config.num_turns;
    }
    std::cout << " | B:" << std::setw(5) << stats.black_wins
              << " W:" << std::setw(5) << stats.white_wins
              << " D:" << std::setw(4) << stats.draws
              << " | NTuple:" << std::setw(5) << std::setprecision(1) << ntuple_win_rate << "%";
    
    if (stats.recent_wins.size() >= 100) {
        std::cout << " (R" << stats.recent_wins.size() << ":" 
                  << std::setw(4) << std::setprecision(1) << recent_1000_win_rate << "%)";
    }
    
    std::cout << " | Opp:" << opp.current_type
              << " | LR:" << std::setw(6) << std::fixed << std::setprecision(4) << display_lr
              << " | " << std::setw(4) << std::setprecision(1) << avg_moves << "m"
              << " | " << std::setw(5) << std::setprecision(1) << games_per_sec << " g/s";
    
    if (stats.recent_games_count > 0) {
        std::cout << " | Updates B:" << stats.recent_black_updates 
                  << " W:" << stats.recent_white_updates;
    }
    std::cout << "\n";
}

/**
 * チェックポイントを保存
 */
void save_checkpoint(
    NTupleNetwork& network,
    OpponentState& opp,
    const std::string& save_path,
    int game
) {
    std::string checkpoint_path = save_path + "." + std::to_string(game);
    network.save(checkpoint_path);
    std::cout << "Saved checkpoint: " << checkpoint_path << "\n";
    
    // デバッグ: 評価値サンプル表示
    GameState debug_state;
    debug_state.reset();
    float initial_eval = network.evaluate(debug_state);
    std::cout << "  Debug: Initial position eval = " << initial_eval 
              << " (current_player=" << (debug_state.current_player() == Player::Black ? "Black" : "White") << ")\n";
}

/**
 * 最終統計を表示
 */
void print_final_statistics(
    const TrainingConfig& config,
    const TrainingStats& stats,
    std::chrono::steady_clock::time_point start_time
) {
    auto end_time = std::chrono::steady_clock::now();
    auto total_elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        end_time - start_time).count();
    
    std::cout << "\nTraining complete!\n";
    std::cout << "Total time: " << total_elapsed << " seconds\n";
    if (config.num_turns > 0) {
        std::cout << "Total TD updates (turns): " << stats.total_updates << "\n";
    }

    int denom_games = std::max(1, stats.games_played);
    std::cout << "\nBoard perspective statistics:\n";
    std::cout << "  Black wins: " << stats.black_wins << " (" 
              << (100.0f * stats.black_wins / denom_games) << "%)\n";
    std::cout << "  White wins: " << stats.white_wins << " (" 
              << (100.0f * stats.white_wins / denom_games) << "%)\n";
    std::cout << "  Draws: " << stats.draws << " (" 
              << (100.0f * stats.draws / denom_games) << "%)\n";
    std::cout << "\nN-tuple performance (alternating colors):\n";
    std::cout << "  Wins: " << stats.ntuple_wins << " (" 
              << (100.0f * stats.ntuple_wins / denom_games) << "%)\n";
    std::cout << "  Losses: " << stats.ntuple_losses << " (" 
              << (100.0f * stats.ntuple_losses / denom_games) << "%)\n";
    std::cout << "  Draws: " << stats.ntuple_draws << " (" 
              << (100.0f * stats.ntuple_draws / denom_games) << "%)\n";
    std::cout << "  Average moves per game: " << (stats.total_moves / denom_games) << "\n";
    std::cout << "\nWeights saved to: " << config.save_path << "\n";
}

/**
 * メイン学習ループ（リファクタリング版）
 * 
 * 学習処理を複数の小さな関数に分割し、見通しを良くしたバージョン
 * 
 * @param config 学習設定パラメータ
 */
void train_network(const TrainingConfig& config) {
    // 初期化
    NTupleNetwork network;
    initialize_network(network, config.load_path);
    
    int actual_save_interval = adjust_save_interval(config);
    
    OpponentState opp;
    initialize_opponent(opp, config.opponent);
    
    // N-tupleネットワークの詳細情報とパターンを表示
    std::cout << "\nN-tuple Network Information:\n";
    std::cout << "  Number of tuples: " << network.num_tuples() << "\n";
    std::cout << "  Total weights: " << network.num_weights() << "\n";
    std::cout << "  Memory usage: " << (network.num_weights() * sizeof(float) / (1024.0 * 1024.0)) << " MB\n\n";
    
    std::cout << "N-tuple Patterns:\n";
    const auto& tuples = network.get_tuples();
    for (size_t i = 0; i < tuples.size(); ++i) {
        print_ntuple_pattern(tuples[i], i + 1);
    }
    std::cout << "\n";
    
    print_training_config(config, actual_save_interval);
    
    std::mt19937 rng(std::random_device{}());
    auto start_time = std::chrono::steady_clock::now();
    
    TrainingStats stats;
    const int WIN_RATE_WINDOW = 1000;
    
    auto get_total_steps = [&]() -> long long {
        return (config.num_turns > 0) ? config.num_turns : static_cast<long long>(config.num_games);
    };

    auto get_current_step = [&](int game_index_1based) -> long long {
        if (config.num_turns > 0) {
            return std::min(config.num_turns, stats.total_updates + 1);
        }
        return static_cast<long long>(game_index_1based);
    };

    // メイン学習ループ
    for (int game = 1; game <= config.num_games; ++game) {
        if (config.num_turns > 0 && stats.total_updates >= config.num_turns) {
            break;
        }
        
        // 学習率計算
        long long total_steps = get_total_steps();
        long long current_step = get_current_step(game);
        float current_lr = calculate_learning_rate(current_step, std::max(1LL, total_steps));
        
        // ゲームプレイ（常にリアルタイムセルフプレイ、swap_colorsなし）
        GameResult result = play_training_game(
            network, config, opp.greedy, opp.rulebased, opp.previous, rng, false
        );
        
        // TD学習
        bool update_vs_previous = (opp.previous != nullptr);
        int remaining_updates = -1;
        if (config.num_turns > 0) {
            long long remain_ll = config.num_turns - stats.total_updates;
            remaining_updates = (remain_ll > static_cast<long long>(std::numeric_limits<int>::max()))
                ? std::numeric_limits<int>::max()
                : static_cast<int>(std::max(0LL, remain_ll));
        }
        
        auto [black_upd, white_upd] = td_learn_from_game(
            network, result, current_lr,
            opp.is_vs_greedy || opp.is_vs_rulebased,  // selfの場合は両方の手番を学習
            false, remaining_updates
        );

        stats.total_updates += static_cast<long long>(black_upd) + static_cast<long long>(white_upd);
        stats.games_played++;
        stats.recent_black_updates += black_upd;
        stats.recent_white_updates += white_upd;
        stats.recent_games_count++;
        
        // 統計更新
        if (result.winner == Player::Black) stats.black_wins++;
        else if (result.winner == Player::White) stats.white_wins++;
        else stats.draws++;
        stats.total_moves += result.num_moves;
        
        // NTuple統計更新（vs 固定相手の場合のみカウント、selfの場合は盤面統計のみ）
        Player ntuple_player = Player::Black;  // 固定相手の場合はBlackを担当
        if (opp.is_vs_greedy || opp.is_vs_rulebased) {
            // 固定相手との対戦時のみNTupleの勝率を追跡
            if (result.winner == ntuple_player) {
                stats.ntuple_wins++;
                stats.recent_wins.push_back(true);
            } else if (result.winner != Player::None) {
                stats.ntuple_losses++;
                stats.recent_wins.push_back(false);
            } else {
                stats.ntuple_draws++;
                stats.recent_wins.push_back(false);
            }
        }
        
        // 勝率ウィンドウ維持
        if (stats.recent_wins.size() > WIN_RATE_WINDOW) {
            stats.recent_wins.pop_front();
        }
        
        // 対戦相手切り替えチェック
        if (should_switch_opponent(stats, game)) {
            int recent_win_count = std::count(stats.recent_wins.begin(), stats.recent_wins.end(), true);
            float recent_win_rate = static_cast<float>(recent_win_count) / WIN_RATE_WINDOW;
            switch_opponent(opp, network, game, recent_win_rate);
            stats.recent_wins.clear();
        }
        
        // 進捗表示
        if (game % 10000 == 0) {
            print_progress(config, stats, opp, game, false, start_time);
            stats.recent_black_updates = 0;
            stats.recent_white_updates = 0;
            stats.recent_games_count = 0;
        }

        if (config.num_turns > 0 && stats.total_updates >= config.num_turns) {
            break;
        }
        
        // チェックポイント保存
        if (game % actual_save_interval == 0) {
            save_checkpoint(network, opp, config.save_path, game);
        }
    }
    
    // 最終保存と統計表示
    network.save(config.save_path);
    print_final_statistics(config, stats, start_time);
}

/**
 * メイン関数：コマンドライン引数を解析して学習を開始
 * 
 * サポートされるオプション：
 * --games N          : 学習ゲーム数
 * --lr RATE          : 学習率（実際は動的に変化、この値は参考値）
 * --discount GAMMA   : 割引率（TD(0)では未使用）
 * --epsilon EPS      : ε-greedy の探索率
 * --save-interval N  : チェックポイント保存間隔
 * --output PATH      : 重みの保存先パス
 * --load PATH        : 事前学習済み重みの読み込み
 * --opponent TYPE    : 対戦相手タイプ（self=前回の自分, greedy=Greedy, rulebased=RuleBased）
 * 
 * 実行例：
 *   ./train_ntuple --games 100000 --epsilon 0.2 --opponent self
 *   ./train_ntuple --games 10000 --opponent greedy --load weights.bin
 *   ./train_ntuple --games 50000 --opponent rulebased --output ntuple_vs_rule.bin
 */
int main(int argc, char* argv[]) {
    TrainingConfig config;
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--games" && i + 1 < argc) {
            config.num_games = std::stoi(argv[++i]);
        } else if (arg == "--turns" && i + 1 < argc) {
            config.num_turns = std::stoll(argv[++i]);
        } else if (arg == "--lr" && i + 1 < argc) {
            config.learning_rate = std::stof(argv[++i]);
        } else if (arg == "--discount" && i + 1 < argc) {
            config.discount_factor = std::stof(argv[++i]);
        } else if (arg == "--epsilon" && i + 1 < argc) {
            config.exploration_rate = std::stof(argv[++i]);
        } else if (arg == "--save-interval" && i + 1 < argc) {
            config.save_interval = std::stoi(argv[++i]);
        } else if (arg == "--output" && i + 1 < argc) {
            config.save_path = argv[++i];
        } else if (arg == "--load" && i + 1 < argc) {
            config.load_path = argv[++i];
        } else if (arg == "--opponent" && i + 1 < argc) {
            config.opponent = argv[++i];
        } else if (arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [options]\n";
            std::cout << "Options:\n";
            std::cout << "  --games N          Number of training games (default: 10000)\n";
            std::cout << "  --turns N          Number of TD updates (state updates). If set, training stops when turns reached.\n";
            std::cout << "  --lr RATE          Learning rate (default: 0.01)\n";
            std::cout << "  --discount GAMMA   Discount factor (default: 0.9)\n";
            std::cout << "  --epsilon EPS      Exploration rate (default: 0.1)\n";
            std::cout << "  --save-interval N  Save checkpoint every N games (default: 1000)\n";
            std::cout << "  --output PATH      Output file path (default: ntuple_weights.bin)\n";
            std::cout << "  --load PATH        Load existing weights before training\n";
            std::cout << "  --opponent TYPE    Opponent type: 'self' or 'greedy' (default: self)\n";
            std::cout << "  --help             Show this help message\n";
            return 0;
        }
    }
    
    train_network(config);
    return 0;
}
