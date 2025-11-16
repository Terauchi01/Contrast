#pragma once
#include "contrast/game_state.hpp"
#include "contrast/move.hpp"

namespace contrast_ai {

class MCTS {
public:
  MCTS();
  contrast::Move search(const contrast::GameState& s, int iterations=1000);
};

} // namespace contrast_ai
