#include "contrast_ai/mcts.hpp"
#include "contrast/rules.hpp"
#include "contrast/move_list.hpp"
#include <cmath>
#include <algorithm>
#include <chrono>
#include <iostream>

using namespace contrast_ai;
using namespace contrast;

// ============================================================================
// MCTSNode implementation
// ============================================================================

float MCTSNode::ucb1_value(float exploration_constant) const {
  if (visits == 0) {
    return std::numeric_limits<float>::infinity();
  }
  
  float exploitation = total_value / visits;
  float exploration = exploration_constant * std::sqrt(std::log(parent->visits) / visits);
  
  return exploitation + exploration;
}

bool MCTSNode::is_terminal() const {
  MoveList moves;
  Rules::legal_moves(state, moves);
  
  if (moves.empty()) {
    return true;
  }
  
  if (Rules::is_win(state, Player::Black) || Rules::is_win(state, Player::White)) {
    return true;
  }
  
  return false;
}

MCTSNode* MCTSNode::best_child(float exploration_constant) const {
  MCTSNode* best = nullptr;
  float best_value = -std::numeric_limits<float>::infinity();
  
  for (const auto& child : children) {
    float value = child->ucb1_value(exploration_constant);
    if (value > best_value) {
      best_value = value;
      best = child.get();
    }
  }
  
  return best;
}

// ============================================================================
// MCTS implementation
// ============================================================================

MCTS::MCTS() 
  : rng_(std::random_device{}()), exploration_constant_(1.414f), verbose_(false) {
}

MCTS::MCTS(const std::string& ntuple_weights_file)
  : rng_(std::random_device{}()), exploration_constant_(1.414f), verbose_(false) {
  load_network(ntuple_weights_file);
}

void MCTS::set_network(const NTupleNetwork& network) {
  network_ = network;
}

void MCTS::load_network(const std::string& weights_file) {
  network_.load(weights_file);
}

std::vector<Move> MCTS::get_legal_moves(const GameState& state) const {
  MoveList moves;
  Rules::legal_moves(state, moves);
  
  std::vector<Move> result;
  result.reserve(moves.size);
  for (size_t i = 0; i < moves.size; ++i) {
    result.push_back(moves[i]);
  }
  
  return result;
}

bool MCTS::is_terminal(const GameState& state) const {
  MoveList moves;
  Rules::legal_moves(state, moves);
  
  if (moves.empty()) {
    return true;
  }
  
  if (Rules::is_win(state, Player::Black) || Rules::is_win(state, Player::White)) {
    return true;
  }
  
  return false;
}

float MCTS::evaluate_state(const GameState& state) const {
  return network_.evaluate(state);
}

/**
 * Selection: UCB1を使って探索すべきノードを選択
 */
MCTSNode* MCTS::selection(MCTSNode* node) {
  while (!node->is_terminal() && node->is_fully_expanded) {
    node = node->best_child(exploration_constant_);
  }
  return node;
}

/**
 * Expansion: 新しい子ノードを展開
 */
MCTSNode* MCTS::expansion(MCTSNode* node) {
  if (node->is_terminal()) {
    return node;
  }
  
  // 初回訪問時：未試行の手をリストアップ
  if (node->untried_moves.empty() && !node->is_fully_expanded) {
    node->untried_moves = get_legal_moves(node->state);
  }
  
  // 未試行の手があれば展開
  if (!node->untried_moves.empty()) {
    // ランダムに手を選択（または評価値順）
    std::uniform_int_distribution<size_t> dist(0, node->untried_moves.size() - 1);
    size_t idx = dist(rng_);
    Move move = node->untried_moves[idx];
    node->untried_moves.erase(node->untried_moves.begin() + idx);
    
    // 新しいノードを作成
    GameState new_state = node->state;
    new_state.apply_move(move);
    
    auto child = std::make_unique<MCTSNode>(new_state, move, node);
    MCTSNode* child_ptr = child.get();
    node->children.push_back(std::move(child));
    
    // 全ての手を試したら完全展開フラグを立てる
    if (node->untried_moves.empty()) {
      node->is_fully_expanded = true;
    }
    
    return child_ptr;
  }
  
  return node;
}

/**
 * Simulation: N-tuple評価を使用（ランダムプレイアウトの代わり）
 */
float MCTS::simulation(const GameState& state) {
  // 終局チェック
  if (is_terminal(state)) {
    if (Rules::is_win(state, Player::Black)) {
      return (state.current_player() == Player::Black) ? 1.0f : -1.0f;
    } else if (Rules::is_win(state, Player::White)) {
      return (state.current_player() == Player::White) ? 1.0f : -1.0f;
    } else {
      return 0.0f;  // Draw
    }
  }
  
  // N-tuple評価を使用（高速シミュレーション）
  return evaluate_state(state);
}

/**
 * Backpropagation: 評価値を親ノードに伝播
 */
void MCTS::backpropagation(MCTSNode* node, float value) {
  while (node != nullptr) {
    node->visits++;
    // Negamax形式: 手番が変わるごとに符号反転
    node->total_value += value;
    value = -value;
    node = node->parent;
  }
}

/**
 * メイン探索関数
 */
Move MCTS::search(const GameState& s, int iterations) {
  auto start_time = std::chrono::steady_clock::now();
  
  // ルートノード作成
  auto root = std::make_unique<MCTSNode>(s);
  
  // MCTS反復
  for (int i = 0; i < iterations; ++i) {
    // 1. Selection
    MCTSNode* node = selection(root.get());
    
    // 2. Expansion
    node = expansion(node);
    
    // 3. Simulation
    float value = simulation(node->state);
    
    // 4. Backpropagation
    backpropagation(node, value);
  }
  
  // 最も訪問回数が多い子ノードを選択
  MCTSNode* best = nullptr;
  int best_visits = -1;
  
  for (const auto& child : root->children) {
    if (child->visits > best_visits) {
      best_visits = child->visits;
      best = child.get();
    }
  }
  
  auto end_time = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
  
  if (verbose_ && best != nullptr) {
    float win_rate = (best->total_value / best->visits + 1.0f) / 2.0f * 100.0f;
    std::cout << "[MCTS] Iterations: " << iterations
              << " | Best move visits: " << best->visits
              << " | Win rate: " << win_rate << "%"
              << " | Time: " << duration << "ms\n";
  }
  
  return best ? best->move_from_parent : Move();
}

/**
 * 思考時間指定で探索
 */
Move MCTS::search_time(const GameState& s, int milliseconds) {
  auto start_time = std::chrono::steady_clock::now();
  auto root = std::make_unique<MCTSNode>(s);
  
  int iterations = 0;
  while (true) {
    auto current_time = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count();
    
    if (elapsed >= milliseconds) {
      break;
    }
    
    // MCTS 1反復
    MCTSNode* node = selection(root.get());
    node = expansion(node);
    float value = simulation(node->state);
    backpropagation(node, value);
    
    iterations++;
  }
  
  // 最良手を選択
  MCTSNode* best = nullptr;
  int best_visits = -1;
  
  for (const auto& child : root->children) {
    if (child->visits > best_visits) {
      best_visits = child->visits;
      best = child.get();
    }
  }
  
  if (verbose_ && best != nullptr) {
    float win_rate = (best->total_value / best->visits + 1.0f) / 2.0f * 100.0f;
    std::cout << "[MCTS] Iterations: " << iterations
              << " | Best move visits: " << best->visits
              << " | Win rate: " << win_rate << "%"
              << " | Time: " << milliseconds << "ms\n";
  }
  
  return best ? best->move_from_parent : Move();
}
