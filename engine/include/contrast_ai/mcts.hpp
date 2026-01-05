#pragma once
#include "contrast/game_state.hpp"
#include "contrast/move.hpp"
#include "contrast_ai/ntuple_big.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <random>

namespace contrast_ai {

/**
 * MCTSノード
 * ゲーム木の各ノードを表現
 */
struct MCTSNode {
  contrast::GameState state;
  contrast::Move move_from_parent;  // 親から遷移してきた手
  MCTSNode* parent;
  std::vector<std::unique_ptr<MCTSNode>> children;
  
  int visits;
  float total_value;  // 累積評価値
  bool is_fully_expanded;
  std::vector<contrast::Move> untried_moves;
  
  MCTSNode(const contrast::GameState& s, contrast::Move m = contrast::Move(), MCTSNode* p = nullptr)
    : state(s), move_from_parent(m), parent(p), visits(0), total_value(0.0f), is_fully_expanded(false) {}
  
  float ucb1_value(float exploration_constant = 1.414f) const;
  bool is_terminal() const;
  MCTSNode* best_child(float exploration_constant) const;
};

/**
 * MCTS (Monte Carlo Tree Search)
 * N-tupleネットワークを評価関数として使用
 */
class MCTS {
public:
  MCTS();
  explicit MCTS(const std::string& ntuple_weights_file);
  
  // 探索を実行して最良手を返す
  contrast::Move search(const contrast::GameState& s, int iterations=1000);
  
  // 思考時間を指定して探索
  contrast::Move search_time(const contrast::GameState& s, int milliseconds);
  
  // N-tupleネットワークを設定
  void set_network(const NTupleNetwork& network);
  void load_network(const std::string& weights_file);
  
  // パラメータ設定
  void set_exploration_constant(float c) { exploration_constant_ = c; }
  void set_verbose(bool v) { verbose_ = v; }
  
private:
  // MCTSの4つのフェーズ
  MCTSNode* selection(MCTSNode* node);
  MCTSNode* expansion(MCTSNode* node);
  float simulation(const contrast::GameState& state);
  void backpropagation(MCTSNode* node, float value);
  
  // ヘルパー関数
  std::vector<contrast::Move> get_legal_moves(const contrast::GameState& state) const;
  bool is_terminal(const contrast::GameState& state) const;
  float evaluate_state(const contrast::GameState& state) const;
  
  NTupleNetwork network_;
  std::mt19937 rng_;
  float exploration_constant_;
  bool verbose_;
};

} // namespace contrast_ai
