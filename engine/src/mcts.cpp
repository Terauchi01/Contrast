#include "contrast_ai/mcts.hpp"
using namespace contrast_ai;

MCTS::MCTS() {}

contrast::Move MCTS::search(const contrast::GameState&, int) {
  return {0,0};
}
