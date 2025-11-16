#include "contrast_ai/ntuple.hpp"
#include "contrast/rules.hpp"
#include "contrast/board.hpp"
#include "contrast/move_list.hpp"
#include "contrast/symmetry.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <chrono>

namespace contrast_ai {

// ============================================================================
// NTuple implementation
// ============================================================================

/**
 * セルの状態を0-8の整数にエンコード
 * 
 * 盤面の各セルは「駒の状態（occupant）」と「タイルの色（tile）」を持つ
 * これらを1つの整数に変換することで、パターン全体を1つのインデックスで表現できる
 * 
 * エンコード方式：
 *   occupant (0=空, 1=黒, 2=白) × 3 + tile (0=なし, 1=黒タイル, 2=灰タイル)
 *   例：
 *     - 空のマス + タイルなし = 0*3 + 0 = 0
 *     - 黒駒 + 黒タイル = 1*3 + 1 = 4
 *     - 白駒 + 灰タイル = 2*3 + 2 = 8
 * 
 * @param c エンコードするセル
 * @return 0-8の整数（9通りの状態）
 */
int NTuple::encode_cell(const contrast::Cell& c) {
  int occ = static_cast<int>(c.occupant);
  int tile = static_cast<int>(c.tile);
  return occ * 3 + tile;
}

/**
 * タイル残数を0-7にエンコード
 * 
 * エンコード方式: 黒タイル + 灰色タイル × 4
 * 
 * 例:
 *   - 黒3枚、灰1枚（初期状態） → 3 + 1×4 = 7
 *   - 黒2枚、灰1枚 → 2 + 1×4 = 6
 *   - 黒3枚、灰0枚 → 3 + 0×4 = 3
 *   - 黒0枚、灰0枚（使い切り） → 0 + 0×4 = 0
 * 
 * @param black_tiles 黒タイル残数 (0-3)
 * @param gray_tiles 灰色タイル残数 (0-1)
 * @return エンコードされた値 (0-7)
 */
int NTuple::encode_tile_inventory(int black_tiles, int gray_tiles) {
  return black_tiles + gray_tiles * 4;
}

/**
 * パターンの完全なゲーム状態を一意のインデックスに変換
 * 
 * v2.0拡張: 盤面パターン + タイル残数情報
 * 
 * インデックス計算式:
 *   idx = board_pattern_idx × 64 + tile_idx
 *   
 * tile_idxの構成:
 *   - 黒プレイヤーのタイル情報: 0-7 (8通り)
 *   - 白プレイヤーのタイル情報: 0-7 (8通り)
 *   - 組み合わせ: black_tile_idx × 8 + white_tile_idx
 * 
 * 仕組み：
 *   - パターン内の各セルを9進数の「桁」として扱う
 *   - 左から順に処理し、base=9で桁上げしながら結合
 *   - 最後にタイル情報（64通り）を追加
 * 
 * メモリ使用量の見積もり（v2.0）：
 *   - 3x3（9セル）: 9^9 × 64 = 24,794,911,296 状態 → 約92GB (float)
 *   - 2x2（4セル）: 9^4 × 64 = 419,904 状態 → 約1.6MB (float)
 *   - 推奨: より小さなパターンを複数使用
 * 
 * @param state 評価するゲーム状態（盤面+タイル残数）
 * @param offset_x パターンのX方向オフセット
 * @param offset_y パターンのY方向オフセット
 * @return パターン状態を表す一意のインデックス（0 ～ num_states()-1）
 */
long long NTuple::to_index(const contrast::GameState& state, int offset_x, int offset_y) const {
  const contrast::Board& b = state.board();
  long long idx = 0;
  constexpr int base = 9; // 9 possible states per cell
  
  // Encode board pattern
  for (size_t i = 0; i < num_cells; ++i) {
    int cell_idx = cell_indices[i];
    int dx = cell_idx % 5;
    int dy = cell_idx / 5;
    int x = offset_x + dx;
    int y = offset_y + dy;
    
    // Out of bounds = treat as empty
    if (x < 0 || x >= b.width() || y < 0 || y >= b.height()) {
      idx = idx * base + 0;
    } else {
      idx = idx * base + encode_cell(b.at(x, y));
    }
  }
  
  // Encode tile inventory (2 slots: black player, white player)
  // Each slot: 0-7 (black_tiles + gray_tiles*4)
  const contrast::TileInventory& black_inv = state.inventory(contrast::Player::Black);
  const contrast::TileInventory& white_inv = state.inventory(contrast::Player::White);
  
  int black_tile_idx = encode_tile_inventory(black_inv.black, black_inv.gray);
  int white_tile_idx = encode_tile_inventory(white_inv.black, white_inv.gray);
  
  // Combine: board_pattern × 64 + (black_tile_idx × 8 + white_tile_idx)
  idx = idx * 64 + (black_tile_idx * 8 + white_tile_idx);
  
  return idx;
}

/**
 * このパターンが取りうる状態の総数を計算
 * 
 * v2.0拡張: 盤面パターン × タイル情報
 * 
 * 重みテーブルのサイズを決定するために使用
 * 各セルが9通り → Nセルなら 9^N 通り
 * タイル情報が 8×8 = 64通り
 * 合計: 9^N × 64
 * 
 * メモリ使用量の見積もり（v2.0）：
 *   - 3x3（9セル）: 9^9 × 64 = 24,794,911,296 状態 → 約92GB (float)
 *   - 2x2（4セル）: 9^4 × 64 = 419,904 状態 → 約1.6MB (float)
 *   - 2x3（6セル）: 9^6 × 64 = 33,849,984 状態 → 約129MB (float)
 *   - 推奨: 2x2や2x3など小さめのパターンを複数組み合わせる
 * 
 * @return 状態の総数
 */
long long NTuple::num_states() const {
  long long result = 1;
  for (size_t i = 0; i < num_cells; ++i) {
    result *= 9LL;
  }
  // Multiply by tile inventory combinations (8 × 8 = 64)
  result *= 64LL;
  return result;
}

// ============================================================================
// NTupleNetwork implementation
// ============================================================================

/**
 * NTupleNetworkのコンストラクタ
 * 
 * ネットワークの初期化手順：
 *   1. パターンの定義（init_tuples）
 *   2. 各パターンの重みテーブルを0で初期化
 * 
 * 重みテーブル構造：
 *   weights_[i][j] = i番目のパターンのj番目の状態に対する評価値
 *   例：weights_[0][12345] = 中央3x3パターンの状態12345の価値
 */
NTupleNetwork::NTupleNetwork() {
  init_tuples();
  
  // Initialize weights to a small positive value
  // Total initial eval = initial_weight * num_patterns = 0.0417 * 12 ≈ 0.5
  // This represents a slight advantage for Black (first player)
  weights_.resize(tuples_.size());
  const float initial_weight = 0.5f / tuples_.size();  // ≈ 0.0417 for 12 patterns
  for (size_t i = 0; i < tuples_.size(); ++i) {
    weights_[i].resize(tuples_[i].num_states(), initial_weight);
  }
}

// Copy constructor for fast in-memory copying
NTupleNetwork::NTupleNetwork(const NTupleNetwork& other) 
  : tuples_(other.tuples_), weights_(other.weights_) {
}

/**
 * 使用するパターンを定義
 * 
 * 8つの基本パターン形状を定義し、それぞれを盤面上の異なる位置に平行移動させて配置
 * これにより位置不変性を高め、盤面全体を効率的にカバーする
 * 
 * 基本パターン：
 *   1. 3x3正方形 - コンパクトな局所領域
 *   2. L字型 - コーナー周辺の特徴
 *   3. T字型 - 縦方向の展開
 *   4-5. 斜め - 斜め方向の連結
 *   6-7. 十字型 - 中央エリアの制御
 *   8. 縦長 - 縦方向の支配
 * 
 * 平行移動戦略：
 *   - 各パターンを盤面上でスライドさせて配置
 *   - はみ出さない範囲で最大限カバー
 *   - 重複を許容（異なる視点から同じ領域を評価）
 * 
 * メモリ使用量：
 *   - 各パターン: 9^9 = 387,420,489 状態 → 約1.44GB
 *   - 合計パターン数に応じて線形増加
 */
void NTupleNetwork::init_tuples() {
  // 12個の基本パターンを定義
  std::vector<std::vector<int>> base_patterns = {
    // 横長パターン（5x2形状）
    {0, 1, 2, 3, 4, 5, 6, 7, 8},         // 横長 行0-1
    {5, 6, 7, 8, 9, 10, 11, 12, 13},     // 横長 行1-2
    {10, 11, 12, 13, 14, 15, 16, 17, 18}, // 横長 行2-3
    {15, 16, 17, 18, 19, 20, 21, 22, 23}, // 横長 行3-4
    
    // 3x3正方形パターン
    {0, 1, 2, 5, 6, 7, 10, 11, 12},      // 3x3 左上
    {1, 2, 3, 6, 7, 8, 11, 12, 13},      // 3x3 中上
    {5, 6, 7, 10, 11, 12, 15, 16, 17},   // 3x3 左中
    {6, 7, 8, 11, 12, 13, 16, 17, 18},   // 3x3 中央
    {10, 11, 12, 15, 16, 17, 20, 21, 22}, // 3x3 左下
    {11, 12, 13, 16, 17, 18, 21, 22, 23}, // 3x3 中下
    
    // T字型・斜めパターン
    {0, 1, 2, 3, 4, 5, 10, 15, 20},      // T字型
    {0, 1, 2, 3, 4, 7, 12, 17, 22},      // 斜め
  };
  
  long long total_states = 0;
  int total_patterns = 0;
  
  // 基本パターンのみを追加（左右反転は評価・学習時に処理）
  for (const auto& base : base_patterns) {
    NTuple pattern;
    pattern.num_cells = 0;
    for (int cell : base) {
      pattern.cell_indices[pattern.num_cells++] = cell;
    }
    tuples_.push_back(pattern);
    total_patterns++;
    if (total_states == 0) {
      total_states = pattern.num_states();
    }
  }
  
  std::cout << "Initializing N-tuple network with symmetry...\n";
  std::cout << "Base patterns: " << base_patterns.size() << "\n";
  std::cout << "  - 4x horizontal patterns (5x2)\n";
  std::cout << "  - 6x 3x3 square patterns\n";
  std::cout << "  - 2x special patterns (T-shape, diagonal)\n";
  std::cout << "Note: Left-right flip handled during evaluation/learning\n";
  std::cout << "Total patterns: " << total_patterns << "\n";
  std::cout << "Cells per pattern: 9\n";
  std::cout << "States per pattern: " << total_states << "\n";
  std::cout << "Total memory usage: ~" 
            << (total_patterns * total_states * sizeof(float) / (1024.0*1024.0*1024.0)) 
            << " GB\n";
}

/**
 * 盤面状態から特徴インデックスを抽出（内部用）
 * 
 * v2.0: タイル情報を含む完全な状態をエンコード
 * 
 * 各パターンについて、現在の盤面状態+タイル残数をインデックスに変換
 * これらのインデックスが重みテーブルへのアクセスキーとなる
 * 
 * @param state 評価する局面（盤面+タイル情報）
 * @return 各パターンのインデックスのリスト
 */
std::vector<int> NTupleNetwork::extract_features(const contrast::GameState& state) const {
  std::vector<int> features;
  features.reserve(tuples_.size());
  
  // Each tuple evaluated at its base position
  // Now includes tile inventory information
  for (const auto& tuple : tuples_) {
    int idx = tuple.to_index(state, 0, 0);
    features.push_back(idx);
  }
  
  return features;
}

/**
 * 盤面を評価して価値を返す（メイン評価関数）
 * 
 * v2.0拡張: タイル残数を考慮した評価
 * 
 * N-tupleネットワークの推論プロセス：
 * 
 * 1. 対称性の正規化
 *    - 同じ局面でも向きが違うと別物として学習してしまう問題を解決
 *    - 8種類の対称性（回転・反転）から代表形を選択
 *    - 例：盤面を180度回転しても本質的には同じ → 1つの形に統一
 * 
 * 2. 完全な状態のエンコード（v2.0）
 *    - 盤面パターン + タイル残数情報
 *    - 終盤でタイルが少ない状況を正しく評価可能
 * 
 * 3. パターンマッチング
 *    - 各パターン（現在は3x3のみ）について、正規化された盤面+タイル情報から
 *      インデックスを計算し、対応する重みを取得
 * 
 * 4. 価値の合算
 *    - 全パターンの重みを足し合わせる
 *    - 線形結合：value = Σ weights[pattern_i][index_i]
 * 
 * 5. 手番の考慮
 *    - 重みは常に黒番視点で学習
 *    - 白番の場合は符号を反転（白にとって良い = 黒にとって悪い）
 * 
 * @param state 評価する局面（盤面+タイル情報）
 * @return 評価値（正=現在のプレイヤーに有利、負=不利）
 */
float NTupleNetwork::evaluate(const contrast::GameState& state) const {
  // 正規化された（対称性を考慮した）ボードを使用
  const auto& b = state.board();
  auto canonical_sym = contrast::SymmetryOps::get_canonical_symmetry(b);
  auto canonical_board = contrast::SymmetryOps::transform_board(b, canonical_sym);
  
  // Create canonical state with transformed board but same tile inventory
  contrast::GameState canonical_state = state;
  canonical_state.board() = canonical_board;
  
  float value = 0.0f;
  
  // 全パターンの重みを足し合わせる（タイル情報を含む）
  for (size_t i = 0; i < tuples_.size(); ++i) {
    const auto& tuple = tuples_[i];
    const auto& weights = weights_[i];
    
    int idx = tuple.to_index(canonical_state, 0, 0);
    value += weights[idx];
  }
  
  // Flip sign if evaluating from White's perspective
  if (state.current_player() == contrast::Player::White) {
    value = -value;
  }
  
  return value;
}

/**
 * TD学習で重みを更新（学習のコア）
 * 
 * v2.0拡張: タイル情報を含む完全な状態で学習
 * 
 * Temporal Difference (TD) Learning の実装
 * 強化学習の一種で、時系列データから価値を学習する手法
 * 
 * 学習プロセス：
 * 
 * 1. 現在の評価値を計算
 *    - 正規化された盤面+タイル情報から、現在のネットワークが予測する価値を取得
 * 
 * 2. 誤差の計算
 *    - error = target - current_value
 *    - target: 教師信号（報酬 + 次の状態の価値、または最終的な勝敗）
 *    - current_value: 現在のネットワークの予測
 * 
 * 3. 手番の考慮
 *    - 評価は手番に応じて符号反転される
 *    - 更新時も一貫性を保つため、誤差を黒番視点に戻す
 * 
 * 4. 重みの更新
 *    - weights[idx] += learning_rate * error
 *    - 誤差が大きいほど大きく更新
 *    - 学習率で更新幅を制御
 * 
 * 5. 正規化
 *    - 学習率をパターン数で割る（複数パターン使用時の安定化）
 * 
 * TD(0)の特徴：
 *   - 1手先の予測との誤差で学習（TD(0)）
 *   - ゲーム終了時は最終結果（勝敗）で学習
 *   - モンテカルロ法より効率的（全ゲーム終了を待たない）
 * 
 * @param state 更新対象の局面（盤面+タイル情報）
 * @param target 教師信号（目標となる評価値）
 * @param learning_rate 学習率（更新の大きさを制御、通常0.001-0.01）
 */
void NTupleNetwork::td_update(const contrast::GameState& state, float target, float learning_rate) {
  // 正規化された（対称性を考慮した）ボードを使用
  const auto& b = state.board();
  auto canonical_sym = contrast::SymmetryOps::get_canonical_symmetry(b);
  auto canonical_board = contrast::SymmetryOps::transform_board(b, canonical_sym);
  
  // Create canonical state with transformed board but same tile inventory
  contrast::GameState canonical_state = state;
  canonical_state.board() = canonical_board;
  
  // Get current value in raw form (before perspective flip)
  float raw_value = 0.0f;
  for (size_t i = 0; i < tuples_.size(); ++i) {
    const auto& tuple = tuples_[i];
    const auto& weights = weights_[i];
    
    int idx = tuple.to_index(canonical_state, 0, 0);
    raw_value += weights[idx];
  }
  
  // Apply perspective for current player
  float current_value = raw_value;
  if (state.current_player() == contrast::Player::White) {
    current_value = -current_value;
  }
  
  float error = target - current_value;
  
  // Convert error back to raw (Black's perspective) for weight update
  if (state.current_player() == contrast::Player::White) {
    error = -error;
  }
  
  // Normalize learning rate by number of tuples
  float normalized_lr = learning_rate / tuples_.size();
  
  // Update all tuples (with tile inventory information)
  for (size_t i = 0; i < tuples_.size(); ++i) {
    const auto& tuple = tuples_[i];
    auto& weights = weights_[i];
    
    int idx = tuple.to_index(canonical_state, 0, 0);
    weights[idx] += normalized_lr * error;
  }
}

/**
 * 学習済み重みをファイルに保存
 * 
 * バイナリ形式で保存（テキストより高速・省容量）
 * 
 * ファイル構造：
 *   1. パターン数（size_t）
 *   2. 各パターンについて：
 *      - 重みテーブルのサイズ（size_t）
 *      - 重みデータ（float配列）
 * 
 * 3x3パターン1つの場合：
 *   - パターン数: 1
 *   - サイズ: 387,420,489
 *   - データ: 387,420,489 floats (約1.44GB)
 * 
 * @param filename 保存先ファイル名
 */
void NTupleNetwork::save(const std::string& filename) const {
  std::ofstream ofs(filename, std::ios::binary);
  if (!ofs) return;
  
  // Save number of tuples
  size_t num_tuples = tuples_.size();
  ofs.write(reinterpret_cast<const char*>(&num_tuples), sizeof(num_tuples));
  
  // Save each tuple's weights
  for (const auto& w : weights_) {
    size_t size = w.size();
    ofs.write(reinterpret_cast<const char*>(&size), sizeof(size));
    ofs.write(reinterpret_cast<const char*>(w.data()), size * sizeof(float));
  }
}

/**
 * 保存された重みをファイルから読み込み
 * 
 * 学習済みモデルを再利用するために必須
 * 継続学習や評価に使用
 * 
 * 安全性チェック：
 *   - パターン数が一致するか確認
 *   - 不一致の場合は読み込まない（構造が異なる）
 * 
 * @param filename 読み込むファイル名
 */
void NTupleNetwork::load(const std::string& filename) {
  std::ifstream ifs(filename, std::ios::binary);
  if (!ifs) return;
  
  size_t num_tuples;
  ifs.read(reinterpret_cast<char*>(&num_tuples), sizeof(num_tuples));
  
  if (num_tuples != tuples_.size()) return; // Mismatch
  
  for (auto& w : weights_) {
    size_t size;
    ifs.read(reinterpret_cast<char*>(&size), sizeof(size));
    w.resize(size);
    ifs.read(reinterpret_cast<char*>(w.data()), size * sizeof(float));
  }
}

/**
 * 重みの総数を取得
 * 
 * メモリ使用量の確認やデバッグに使用
 * 
 * @return 全パターンの重みの合計数
 */
size_t NTupleNetwork::num_weights() const {
  size_t total = 0;
  for (const auto& w : weights_) {
    total += w.size();
  }
  return total;
}

// ============================================================================
// NTuplePolicy implementation
// ============================================================================

NTuplePolicy::NTuplePolicy() 
  : rng_(std::chrono::steady_clock::now().time_since_epoch().count()) {
}

NTuplePolicy::NTuplePolicy(const std::string& weights_file)
  : rng_(std::chrono::steady_clock::now().time_since_epoch().count()) {
  network_.load(weights_file);
}

contrast::Move NTuplePolicy::pick(const contrast::GameState& s) {
  contrast::MoveList moves;
  contrast::Rules::legal_moves(s, moves);
  if (moves.empty()) {
    return contrast::Move(); // No legal move
  }
  
  // Evaluate all moves
  float best_value = -1e9f;
  contrast::MoveList best_moves;
  
  for (size_t i = 0; i < moves.size; ++i) {
    const auto& m = moves[i];
    // Apply move to get next state
    contrast::GameState next = s;
    next.apply_move(m);
    
    // Evaluate from current player's perspective
    // (network already handles perspective flip)
    float value = -network_.evaluate(next); // Negamax: opponent's value = -our value
    
    if (value > best_value) {
      best_value = value;
      best_moves.clear();
      best_moves.push_back(m);
    } else if (std::abs(value - best_value) < 1e-6f) {
      best_moves.push_back(m);
    }
  }
  
  // Pick randomly among best moves
  if (best_moves.empty()) return moves[0];
  
  std::uniform_int_distribution<size_t> dist(0, best_moves.size - 1);
  return best_moves[dist(rng_)];
}

bool NTuplePolicy::save(const std::string& filename) const {
  network_.save(filename);
  return true;
}

bool NTuplePolicy::load(const std::string& filename) {
  try {
    network_.load(filename);
    return true;
  } catch (const std::exception& e) {
    std::cerr << "Failed to load N-tuple weights: " << e.what() << "\n";
    return false;
  }
}

} // namespace contrast_ai
