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

using namespace contrast;
using namespace contrast_ai;

/**
 * 学習の設定パラメータ
 * 
 * コマンドライン引数で上書き可能な学習設定を保持
 */
struct TrainingConfig {
    int num_games = 10000;              // 総学習ゲーム数
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
    bool swap_colors = false
) {
    if (result.states.empty()) return {0, 0};
    
    // デバッグ用：更新された手数をカウント
    int black_updates = 0;
    int white_updates = 0;
    
    // Backward updates from end to start
    // For each state, we assign reward based on final outcome from that player's perspective
    for (int i = result.states.size() - 1; i >= 0; --i) {
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
float calculate_learning_rate(int current_game, int total_games, float lr_max = 0.1f, float lr_min = 0.005f) {
    float progress = static_cast<float>(current_game - 1) / static_cast<float>(total_games - 1);
    
    // 逆数ベースの減衰（序盤急速、後半緩やか）
    // k=19 で、progress=0.5のとき約lr=0.025（中間値）になる
    float k = 19.0f;
    float lr = lr_min + (lr_max - lr_min) / (1.0f + k * progress * progress);
    
    return lr;
}

/**
 * メイン学習ループ
 * 
 * 指定されたゲーム数だけセルフプレイを行い、TD学習で重みを更新
 * 
 * 主要な処理：
 * 1. ネットワークの初期化（または既存重みの読み込み）
 * 2. 対戦相手の設定（セルフプレイ/Greedy/前回のチェックポイント）
 * 3. 各ゲームのプレイと学習
 * 4. 学習率の動的調整（逆二乗減衰）
 * 5. チェックポイント保存と先手後手の入れ替え
 * 6. 統計情報の表示（勝率、手数、学習速度など）
 * 
 * 最適化：
 * - メモリ内コピーで前回ネットワークを保持（ディスクI/O削減）
 * - save_intervalごとに先手後手を入れ替え（学習バランス）
 * - 自動save_interval調整（総ゲーム数の1/10）
 * 
 * @param config 学習設定パラメータ
 */
void train_network(const TrainingConfig& config) {
    NTupleNetwork network;
    NTupleNetwork* previous_network = nullptr;  // 前回のチェックポイント
    
    // Automatically set save_interval to 1/10th of total games if not explicitly set
    int actual_save_interval = config.save_interval;
    if (config.save_interval == 1000) {  // Default value
        actual_save_interval = std::max(100, config.num_games / 10);
        std::cout << "Auto-adjusting save interval to " << actual_save_interval 
                  << " (1/10th of total games)\n";
    }
    
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
    
    std::mt19937 rng(std::random_device{}());
    
    // Create opponents if needed
    GreedyPolicy* greedy_opponent = nullptr;
    RuleBasedPolicy* rulebased_opponent = nullptr;
    bool is_vs_greedy = (config.opponent == "greedy");
    bool is_vs_rulebased = (config.opponent == "rulebased");
    bool is_vs_previous = (config.opponent == "self");  // セルフプレイ時は前回の自分と対戦
    
    if (is_vs_greedy) {
        greedy_opponent = new GreedyPolicy();
        std::cout << "Training against Greedy opponent\n";
    } else if (is_vs_rulebased) {
        rulebased_opponent = new RuleBasedPolicy();
        std::cout << "Training against RuleBased opponent\n";
    } else if (is_vs_previous) {
        std::cout << "Training with self-play (vs previous checkpoint)\n";
    } else {
        std::cout << "Training with self-play\n";
    }
    
    std::cout << "Starting N-tuple network training\n";
    std::cout << "Configuration:\n";
    std::cout << "  Opponent: " << config.opponent << "\n";
    std::cout << "  Games: " << config.num_games << "\n";
    std::cout << "  Initial learning rate: " << config.learning_rate << "\n";
    std::cout << "  Learning rate schedule: 0.1 -> 0.005 (inverse-square decay)\n";
    std::cout << "    - Fast decay in early games\n";
    std::cout << "    - Gradual decay in later games\n";
    std::cout << "  Discount factor: " << config.discount_factor << "\n";
    std::cout << "  Exploration rate: " << config.exploration_rate << "\n";
    std::cout << "  Save interval: " << actual_save_interval << "\n";
    std::cout << "  Save path: " << config.save_path << "\n\n";
    
    auto start_time = std::chrono::steady_clock::now();
    
    int black_wins = 0;
    int white_wins = 0;
    int draws = 0;
    float total_moves = 0;
    
    // 先手後手の統計（NTupleの視点）
    int ntuple_wins = 0;
    int ntuple_losses = 0;
    int ntuple_draws = 0;
    
    // 先手後手の切替（1万ゲームごと）
    bool swap_colors = false;  // 初期はNTupleが黒（先手）
    std::cout << "Initial colors: NTuple plays as Black\n";
    std::cout << "Colors will swap every 10,000 games\n\n";
    
    // 直近1000試合の勝率追跡（対戦相手切替用）
    std::deque<bool> recent_wins;  // true=勝ち, false=負けまたは引き分け
    const int WIN_RATE_WINDOW = 1000;
    const float WIN_RATE_THRESHOLD = 0.55f;  // 55%で対戦相手切替
    std::string current_opponent_type = config.opponent;  // 現在の対戦相手タイプ
    
    // デバッグ用：直近100ゲームの学習統計
    int recent_black_updates = 0;
    int recent_white_updates = 0;
    int recent_games_count = 0;
    
    for (int game = 1; game <= config.num_games; ++game) {
        // Calculate current learning rate with inverse-square decay
        // 序盤は急速に減衰、中盤以降は緩やかに減衰
        float current_lr = calculate_learning_rate(game, config.num_games);
        
        // Play game
        GameResult result = play_training_game(network, config, greedy_opponent, rulebased_opponent, previous_network, rng, swap_colors);
        
        // Learn from game with current learning rate
        // 前回のネットワークと対戦している場合、学習対象のネットワークの手のみ更新
        bool update_vs_previous = (previous_network != nullptr);
        auto [black_upd, white_upd] = td_learn_from_game(network, result, current_lr, is_vs_greedy || is_vs_rulebased || update_vs_previous, swap_colors);
        
        // 統計を蓄積
        recent_black_updates += black_upd;
        recent_white_updates += white_upd;
        recent_games_count++;
        
        // Update statistics (from board perspective)
        if (result.winner == Player::Black) black_wins++;
        else if (result.winner == Player::White) white_wins++;
        else draws++;
        total_moves += result.num_moves;
        
        // Update NTuple-specific statistics
        Player ntuple_player = swap_colors ? Player::White : Player::Black;
        if (result.winner == ntuple_player) {
            ntuple_wins++;
            recent_wins.push_back(true);
        } else if (result.winner != Player::None) {
            ntuple_losses++;
            recent_wins.push_back(false);
        } else {
            ntuple_draws++;
            recent_wins.push_back(false);  // 引き分けは「勝ち」としてカウントしない
        }
        
        // 直近1000試合の勝率を維持（古い結果を削除）
        if (recent_wins.size() > WIN_RATE_WINDOW) {
            recent_wins.pop_front();
        }
        
        // 直近1000試合の勝率をチェックして対戦相手を切り替え
        if (recent_wins.size() >= WIN_RATE_WINDOW) {
            int recent_win_count = std::count(recent_wins.begin(), recent_wins.end(), true);
            float recent_win_rate = static_cast<float>(recent_win_count) / WIN_RATE_WINDOW;
            
            // 最初の1000試合は無条件で切り替え、その後は55%を超えたら切り替え
            bool should_switch = (game == WIN_RATE_WINDOW) || (recent_win_rate > WIN_RATE_THRESHOLD);
            
            if (should_switch) {
                std::cout << "\n=== ";
                if (game == WIN_RATE_WINDOW) {
                    std::cout << "INITIAL TRAINING COMPLETE";
                } else {
                    std::cout << "WIN RATE THRESHOLD REACHED";
                }
                std::cout << " ===\n";
                std::cout << "Game: " << game << " | Recent " << WIN_RATE_WINDOW << " games win rate: " 
                          << (recent_win_rate * 100.0f) << "%\n";
                std::cout << "Switching opponent from " << current_opponent_type << " to ";
                
                // 対戦相手を切り替え（greedy → rulebased → self）
                if (current_opponent_type == "greedy") {
                    // Greedy → RuleBased
                    current_opponent_type = "rulebased";
                    delete greedy_opponent;
                    greedy_opponent = nullptr;
                    rulebased_opponent = new RuleBasedPolicy();
                    is_vs_greedy = false;
                    is_vs_rulebased = true;
                    is_vs_previous = false;
                    std::cout << "rulebased\n";
                } else if (current_opponent_type == "rulebased") {
                    // RuleBased → Self-play
                    current_opponent_type = "self";
                    delete rulebased_opponent;
                    rulebased_opponent = nullptr;
                    is_vs_greedy = false;
                    is_vs_rulebased = false;
                    is_vs_previous = true;
                    // 現在のネットワークをコピーして対戦相手にする
                    previous_network = new NTupleNetwork(network);
                    std::cout << "self (self-play vs previous checkpoint)\n";
                } else {
                    // すでにself-playの場合は対戦相手を更新するだけ
                    std::cout << "self (updating opponent checkpoint)\n";
                    if (previous_network != nullptr) {
                        delete previous_network;
                    }
                    previous_network = new NTupleNetwork(network);
                }
                
                std::cout << "===================================\n\n";
                
                // 勝率履歴をリセット
                recent_wins.clear();
            }
        }
        
        // Progress reporting
        if (game % 100 == 0) {
            auto current_time = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                current_time - start_time).count();
            
            float avg_moves = total_moves / game;
            float games_per_sec = game / static_cast<float>(elapsed + 1);
            float display_lr = calculate_learning_rate(game, config.num_games);
            float ntuple_win_rate = 100.0f * ntuple_wins / game;
            
            // 直近1000試合の勝率を計算
            float recent_1000_win_rate = 0.0f;
            if (!recent_wins.empty()) {
                int recent_win_count = std::count(recent_wins.begin(), recent_wins.end(), true);
                recent_1000_win_rate = 100.0f * recent_win_count / recent_wins.size();
            }
            
            std::cout << "Game " << std::setw(6) << game << "/" << config.num_games
                      << " | B:" << std::setw(5) << black_wins
                      << " W:" << std::setw(5) << white_wins
                      << " D:" << std::setw(4) << draws
                      << " | NTuple:" << std::setw(5) << std::setprecision(1) << ntuple_win_rate << "%";
            
            // 直近1000試合の勝率を表示
            if (recent_wins.size() >= 100) {  // 最低100試合以上で表示
                std::cout << " (R" << recent_wins.size() << ":" 
                          << std::setw(4) << std::setprecision(1) << recent_1000_win_rate << "%)";
            }
            
            std::cout << " | Opp:" << current_opponent_type
                      << " | LR:" << std::setw(6) << std::fixed << std::setprecision(4) << display_lr
                      << " | " << std::setw(4) << std::setprecision(1) << avg_moves << "m"
                      << " | " << std::setw(5) << std::setprecision(1) << games_per_sec << " g/s";
            
            // デバッグ：直近100ゲームの更新統計を表示
            if (recent_games_count > 0) {
                std::cout << " | Updates B:" << recent_black_updates 
                          << " W:" << recent_white_updates
                          << " (swap=" << (swap_colors ? "W" : "B") << ")";
            }
            std::cout << "\n";
            
            // 統計をリセット
            recent_black_updates = 0;
            recent_white_updates = 0;
            recent_games_count = 0;
        }
        
        // Save checkpoint
        if (game % actual_save_interval == 0) {
            std::string checkpoint_path = config.save_path + "." + std::to_string(game);
            network.save(checkpoint_path);
            std::cout << "Saved checkpoint: " << checkpoint_path << "\n";
            
            // デバッグ: 現在のネットワークの評価値サンプルを表示
            GameState debug_state;
            debug_state.reset();
            float initial_eval = network.evaluate(debug_state);
            std::cout << "  Debug: Initial position eval = " << initial_eval 
                      << " (current_player=" << (debug_state.current_player() == Player::Black ? "Black" : "White") << ")\n";
            
            // 前回のネットワークと対戦する場合、現在のネットワークのコピーを作成（高速なメモリ内コピー）
            if (is_vs_previous) {
                // 古い前回のネットワークを削除
                if (previous_network != nullptr) {
                    delete previous_network;
                }
                // 新しい前回のネットワークとして現在のネットワークのコピーを作成（ディスクI/Oなし）
                previous_network = new NTupleNetwork(network);
                
                // デバッグ: 前回のネットワークと現在のネットワークの評価値が一致するか確認
                float prev_eval = previous_network->evaluate(debug_state);
                std::cout << "  Debug: Previous network eval = " << prev_eval << "\n";
                if (std::abs(initial_eval - prev_eval) > 0.001f) {
                    std::cerr << "  WARNING: Network copy mismatch! Diff = " 
                              << (initial_eval - prev_eval) << "\n";
                }
                
                std::cout << "Updated opponent to latest checkpoint (in-memory copy)\n";
            }
        }
        
        // Swap colors every 10,000 games (independent of save interval)
        const int COLOR_SWAP_INTERVAL = 10000;
        if (game % COLOR_SWAP_INTERVAL == 0) {
            swap_colors = !swap_colors;
            std::cout << "Swapped colors at game " << game 
                      << ": NTuple now plays as " 
                      << (swap_colors ? "White" : "Black") << "\n";
        }
    }
    
    // Final save
    network.save(config.save_path);
    
    // Clean up
    if (greedy_opponent != nullptr) {
        delete greedy_opponent;
    }
    if (rulebased_opponent != nullptr) {
        delete rulebased_opponent;
    }
    if (previous_network != nullptr) {
        delete previous_network;
    }
    
    auto end_time = std::chrono::steady_clock::now();
    auto total_elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        end_time - start_time).count();
    
    std::cout << "\nTraining complete!\n";
    std::cout << "Total time: " << total_elapsed << " seconds\n";
    std::cout << "\nBoard perspective statistics:\n";
    std::cout << "  Black wins: " << black_wins << " (" 
              << (100.0f * black_wins / config.num_games) << "%)\n";
    std::cout << "  White wins: " << white_wins << " (" 
              << (100.0f * white_wins / config.num_games) << "%)\n";
    std::cout << "  Draws: " << draws << " (" 
              << (100.0f * draws / config.num_games) << "%)\n";
    std::cout << "\nN-tuple performance (alternating colors):\n";
    std::cout << "  Wins: " << ntuple_wins << " (" 
              << (100.0f * ntuple_wins / config.num_games) << "%)\n";
    std::cout << "  Losses: " << ntuple_losses << " (" 
              << (100.0f * ntuple_losses / config.num_games) << "%)\n";
    std::cout << "  Draws: " << ntuple_draws << " (" 
              << (100.0f * ntuple_draws / config.num_games) << "%)\n";
    std::cout << "  Average moves per game: " << (total_moves / config.num_games) << "\n";
    std::cout << "\nWeights saved to: " << config.save_path << "\n";
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
