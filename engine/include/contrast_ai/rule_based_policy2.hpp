#pragma once
#include "contrast/game_state.hpp"
#include "contrast/move.hpp"
#include "contrast/move_list.hpp"
#include <vector>
#include <random>

namespace contrast_ai {
// WinCheck() → 勝てるなら動く
// LoseCheck() → 相手の勝利ルートを塞ぐ
// ProgressMove() → 最短距離が縮む移動
// PromoteLeadPiece() → リード駒を優先
// TileBoostMyPiece() → Gray強化など
// TileDisruptOpponent() → 相手の動きを制限
// SelfJumpAccelerate() → 自駒ジャンプで加速
// AntiJumpDefense() → 相手のジャンプ阻止
// AvoidCluster() → 駒配置の安全性確保
// EndgameOpponentBlock() → 終盤妨害最優先

class RuleBasedPolicy2 {
public:
  RuleBasedPolicy2();
  contrast::Move pick(const contrast::GameState& s);
  
private:
  std::mt19937 rng_;
  
  bool WinCheck();
  bool LoseCheck();
  bool ProgressMove();
  bool PromoteLeadPiece();
  bool TileBoostMyPiece();
  bool TileDisruptOpponent();
  bool SelfJumpAccelerate();
  bool AntiJumpDefense();
  bool AvoidCluster();
  bool EndgameOpponentBlock();
  
};

} // namespace contrast_ai
