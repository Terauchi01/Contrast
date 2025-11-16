#pragma once
#include "contrast/game_state.hpp"
#include "contrast/move.hpp"
#include <array>
#include <vector>
#include <random>
#include <string>

namespace contrast_ai {

/**
 * N-tupleパターン定義
 * 
 * N-tupleネットワークの基本単位となる局所パターン
 * 盤面の一部分（例：3x3の領域）の状態を表現
 * 
 * v2.0拡張: タイル残数情報を追加
 *   - 盤面パターン: 9^num_cells 通り
 *   - タイル情報: 8 × 8 = 64通り（黒プレイヤー×白プレイヤー）
 *   - 合計状態数: 9^num_cells × 64
 * 
 * 主な役割：
 *   - どのセルを見るか（cell_indices）を定義
 *   - 盤面状態 + タイル残数を一意のインデックスに変換
 *   - 状態数の計算（メモリ見積もりに使用）
 */
struct NTuple {
  static constexpr size_t MAX_CELLS = 25;  // 5x5盤面の最大セル数
  std::array<int, MAX_CELLS> cell_indices; // パターン内のセル位置（線形化インデックス y*5+x）
  size_t num_cells = 0;                    // パターンに含まれるセルの実際の数
  
  // 完全なゲーム状態（盤面+タイル残数）をこのパターンのインデックスに変換
  // offset: パターンを盤面上で移動させる場合のオフセット
  long long to_index(const contrast::GameState& state, int offset_x, int offset_y) const;
  
  // このパターンが取りうる状態の総数を計算（9^num_cells × 8 × 8）
  long long num_states() const;
  
  // 1セルの状態を0-8の整数にエンコード
  // (occupant, tile) → 単一の整数値
  static int encode_cell(const contrast::Cell& c);
  
  // タイル残数を0-7にエンコード（黒タイル+灰色タイル×4）
  // 黒タイル(0-3) + 灰色タイル(0-1)×4 = 0-7
  static int encode_tile_inventory(int black_tiles, int gray_tiles);
};

/**
 * N-tupleネットワーク - 盤面評価と学習の中核
 * 
 * N-tuple Networkは強化学習で盤面を評価するための関数近似器
 * 複数のパターン（N-tuple）を組み合わせて盤面全体の価値を推定
 * 
 * v2.0拡張: タイル残数を特徴量に追加
 *   - 各パターンがタイル情報を含む
 *   - 終盤戦略の改善に寄与
 * 
 * アーキテクチャ：
 *   - 入力：盤面状態（5x5, 各セル9通り）+ タイル残数（8×8通り）
 *   - 中間：複数のパターン（現在は3x3が1つ）
 *   - 出力：評価値（スカラー、正=有利、負=不利）
 * 
 * 学習方法：
 *   - TD(0)学習（Temporal Difference Learning）
 *   - セルフプレイまたは対戦相手との対局から学習
 *   - 勝敗の結果で最終調整
 * 
 * メモリ使用量（v2.0）：
 *   - 3x3パターン×1個: 9^9 × 64 = 約92GB (float)
 *   - 注意: タイル情報追加でメモリ使用量が64倍に増加
 *   - より小さなパターン推奨（2x2など）
 */
class NTupleNetwork {
public:
  NTupleNetwork();
  
  // Copy constructor for fast in-memory copying
  NTupleNetwork(const NTupleNetwork& other);
  
  // 盤面を評価（現在のプレイヤー視点）
  // 正の値=現在のプレイヤーに有利、負の値=不利
  float evaluate(const contrast::GameState& state) const;
  
  // TD学習で重みを更新
  // target = 報酬 + gamma * 次の状態の価値（または最終勝敗）
  // learning_rate = 更新の大きさ（通常0.001-0.01）
  void td_update(const contrast::GameState& state, float target, float learning_rate);
  
  // 重みの保存/読み込み（学習結果の永続化）
  void save(const std::string& filename) const;
  void load(const std::string& filename);
  
  // パターン数を取得
  size_t num_tuples() const { return tuples_.size(); }
  
  // 重みの総数を取得（デバッグ用）
  size_t num_weights() const;
  
private:
  std::vector<NTuple> tuples_;              // 使用するパターンのリスト
  std::vector<std::vector<float>> weights_; // 各パターンの重みテーブル
  
  // パターンの初期化（現在は3x3を1つ定義）
  void init_tuples();
  
  // 盤面から特徴インデックスを抽出（内部用）
  std::vector<int> extract_features(const contrast::GameState& state) const;
};

/**
 * N-tupleネットワークを使用するポリシー
 * 
 * 学習済みN-tupleネットワークを使って実際にゲームをプレイ
 * 
 * 使用例：
 *   // 学習済みモデルを読み込んで対局
 *   NTuplePolicy policy("weights_10k.bin");
 *   Move move = policy.pick(current_state);
 * 
 * 強化の方向性：
 *   1. ミニマックス探索の追加
 *      - 現在は1手先のみ評価
 *      - 2-3手先まで読めばより強く
 *   2. アルファベータ枝刈り
 *      - 探索の効率化
 *   3. 評価関数の改善
 *      - 勝利判定の追加
 *      - 詰みチェック
 */
class NTuplePolicy {
public:
  // デフォルトコンストラクタ（未学習の状態）
  NTuplePolicy();
  
  // 学習済み重みを読み込んで初期化
  explicit NTuplePolicy(const std::string& weights_file);
  
  // 最良の手を選択
  // 全ての合法手を評価し、評価値が最大のものを返す
  contrast::Move pick(const contrast::GameState& s);
  
  // 重みの保存/読み込み
  bool save(const std::string& filename) const;
  bool load(const std::string& filename);
  
  // 学習用にネットワークへのアクセスを提供
  NTupleNetwork& network() { return network_; }
  const NTupleNetwork& network() const { return network_; }
  
private:
  NTupleNetwork network_; // 評価に使用するネットワーク
  std::mt19937 rng_;      // 同点の手からランダム選択用
};

} // namespace contrast_ai
